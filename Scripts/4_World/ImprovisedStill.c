class ImprovisedStill extends Pot
{
    protected float m_StillProgress;
    protected float m_FermentSeconds;
    protected int m_FermentState; // STILL_FERMENT_*, synced for the tooltip
    static const int STILL_FERMENT_NONE = 0;
    static const int STILL_FERMENT_FERMENTING = 1;
    protected int m_FermentPause; // STILL_PAUSE_*, why fermentation is stalled, synced for the tooltip
    static const int STILL_PAUSE_NONE = 0;
    static const int STILL_PAUSE_COLD = 1;
    static const int STILL_PAUSE_HOT = 2;
    static const int STILL_PAUSE_FRUIT = 3; // not enough fruit for the water
    // A batch saved by a version before 1.1.0, converted to beer on the first
    // tick: STILL_LEGACY_FERMENTING has not had its bad-batch roll yet,
    // STILL_LEGACY_READY has.
    protected int m_LegacyBatch;
    protected const int STILL_LEGACY_NONE = 0;
    protected const int STILL_LEGACY_FERMENTING = 1;
    protected const int STILL_LEGACY_READY = 2;
    protected const float STILL_BATCH_SECONDS = 30.0;
    protected const float STILL_BATCH_QUANTITY = 50.0;
    protected const float STILL_DISTILLATE_TEMPERATURE = 35.0;
    protected const float STILL_PIPE_TEMPERATURE = 60.0;            // with steam going through it
    protected const float STILL_PIPE_CONDUCTION_TEMPERATURE = 40.0; // from the hot pot alone
    protected const int STILL_TICK_FAST_MS = 5000;  // heated: distilling needs it
    protected const int STILL_TICK_SLOW_MS = 30000; // fermenting (a minutes-long timer)

    void ImprovisedStill()
    {
        m_StillProgress = 0.0;
        m_FermentSeconds = 0.0;
        m_FermentState = STILL_FERMENT_NONE;
        m_LegacyBatch = STILL_LEGACY_NONE;
        RegisterNetSyncVariableInt("m_FermentState", 0, 1);
        RegisterNetSyncVariableInt("m_FermentPause", 0, 3);
    }

    int GetFermentState()
    {
        return m_FermentState;
    }

    int GetFermentPause()
    {
        return m_FermentPause;
    }

    // Yeast works best from about 20 °C, slows down as it gets colder and
    // stops near freezing; hot mash (after heating) kills it until it cools.
    protected float GetFermentRate(StillSettings s, out int pause)
    {
        pause = STILL_PAUSE_NONE;
        float temperature = GetTemperature();
        if (temperature > s.FermentMaxTemperature)
        {
            pause = STILL_PAUSE_HOT;
            return 0.0;
        }
        if (temperature <= s.FermentMinTemperature)
        {
            pause = STILL_PAUSE_COLD;
            return 0.0;
        }
        if (temperature >= s.FermentFullSpeedTemperature || s.FermentFullSpeedTemperature <= s.FermentMinTemperature)
            return 1.0;

        return (temperature - s.FermentMinTemperature) / (s.FermentFullSpeedTemperature - s.FermentMinTemperature);
    }

    // Fresh water with fruit or potatoes in the cargo is a mash (fermenting).
    // Once fermented it is beer, a liquid of its own.
    bool HasMash()
    {
        return IsFreshWater() && FindFermentableInput() != null;
    }

    protected bool IsFreshWater()
    {
        int liquid = GetLiquidType();
        return GetQuantity() > 0 && (liquid & LIQUID_GROUP_WATER) && liquid != LIQUID_SALTWATER;
    }

    // To keep idle stills free on busy servers, a still only ticks when it has
    // work: every 5 s while heated, every 30 s while it is fermenting, and not
    // at all when it is empty, ruined, or holds only water or beer. Anything
    // that could give it work wakes it: filling it, adding or removing fruit,
    // moving it (onto a fire, a stove, into a barrel), or lighting the fire or
    // stove it sits on (see StillHeatHooks.c). Each tick picks the next pace.
    protected int m_TickMilliseconds; // 0 = asleep
    protected bool m_TickHeated;
    protected bool m_TickMash;

    protected void SetStillTick(int milliseconds)
    {
        if (milliseconds == m_TickMilliseconds || !g_Game || !g_Game.IsServer())
            return;

        if (m_TickMilliseconds > 0)
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(UpdateStill);
        m_TickMilliseconds = milliseconds;
        if (milliseconds > 0)
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UpdateStill, milliseconds, true);
    }

    // Something may have given the still work; the next tick decides.
    void WakeStill()
    {
        SetStillTick(STILL_TICK_FAST_MS);
    }

    // Wakes any still on or in a heat source that just started.
    static void WakeStillsIn(EntityAI source)
    {
        if (!source || !g_Game || !g_Game.IsServer())
            return;

        for (int i = 0; i < source.GetInventory().AttachmentCount(); i++)
        {
            ImprovisedStill attached = ImprovisedStill.Cast(source.GetInventory().GetAttachmentFromIndex(i));
            if (attached)
                attached.WakeStill();
        }

        CargoBase cargo = source.GetInventory().GetCargo();
        if (!cargo)
            return;
        for (int j = 0; j < cargo.GetItemCount(); j++)
        {
            ImprovisedStill inCargo = ImprovisedStill.Cast(cargo.GetItem(j));
            if (inCargo)
                inCargo.WakeStill();
        }
    }

    override void EEInit()
    {
        super.EEInit();
        WakeStill(); // the first tick puts it back to sleep if it has nothing to do
    }

    override void OnQuantityChanged(float delta)
    {
        super.OnQuantityChanged(delta);
        if (GetQuantity() > 0)
            WakeStill();
    }

    override void EECargoIn(EntityAI item)
    {
        super.EECargoIn(item);
        WakeStill();
    }

    override void EECargoOut(EntityAI item)
    {
        super.EECargoOut(item);
        WakeStill();
    }

    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);
        WakeStill();
    }

    override void EEDelete(EntityAI parent)
    {
        SetStillTick(0);
        super.EEDelete(parent);
    }

    // Save format. The original format starts with m_StillProgress, which is
    // never negative; newer formats start with a negative version marker so
    // old saves can be told apart without a failed read (a failed read marks
    // the entity as corrupted).
    protected const float STILL_SAVE_VERSION_2 = -2.0; // 0.99-1.0.5: adds m_FermentSeconds
    protected const float STILL_SAVE_VERSION_3 = -3.0; // 1.1.0: mash becomes beer; per-fruit tracking gone
    protected const float STILL_OLD_SPOIL_SECONDS = 7200.0; // MashSpoilMinutes' default before 1.1.0

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(STILL_SAVE_VERSION_3);
        ctx.Write(m_StillProgress);
        ctx.Write(m_FermentSeconds);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        float first;
        if (!ctx.Read(first))
            return false;

        m_FermentSeconds = 0.0;
        if (first == STILL_SAVE_VERSION_3)
        {
            if (!ctx.Read(m_StillProgress) || !ctx.Read(m_FermentSeconds))
                return false;
        }
        else
        {
            // Older formats also hold per-fruit alcohol tracking, which 1.1.0
            // no longer uses.
            float produced;
            float target;
            string activeType;
            if (first < 0.0)
            {
                if (!ctx.Read(m_StillProgress))
                    return false;
            }
            else
            {
                m_StillProgress = first; // original format
            }

            if (!ctx.Read(produced) || !ctx.Read(target) || !ctx.Read(activeType))
                return false;

            float oldSeconds = 0.0;
            if (first < 0.0 && !ctx.Read(oldSeconds))
                return false;

            // A batch in progress becomes beer on the first tick; one that had
            // spoiled stays sour water.
            float needed = StillSettings.Get().FermentationMinutes * 60.0;
            if (oldSeconds > 0.0 && oldSeconds < Math.Max(needed, 0.0) + STILL_OLD_SPOIL_SECONDS)
            {
                if (oldSeconds >= needed)
                    m_LegacyBatch = STILL_LEGACY_READY;
                else
                    m_LegacyBatch = STILL_LEGACY_FERMENTING;
            }
        }

        m_StillProgress = Math.Clamp(m_StillProgress, 0.0, STILL_BATCH_SECONDS);
        m_FermentSeconds = Math.Max(m_FermentSeconds, 0.0);
        return true;
    }

    // A still on a fireplace stays heated while the fire burns and while the
    // fireplace is still hot after it goes out (its coals keep heating, as
    // vanilla keeps cooking on a cooling fireplace). A gas stove heats only
    // while it is on.
    bool HasHeatSource()
    {
        EntityAI parent = GetHierarchyParent();
        while (parent)
        {
            FireplaceBase fireplace = FireplaceBase.Cast(parent);
            if (fireplace)
                return fireplace.IsBurning() || fireplace.GetTemperature() >= StillSettings.Get().FireplaceResidualHeatTemperature;

            PortableGasStove stove = PortableGasStove.Cast(parent);
            if (stove)
                return stove.GetCompEM() && stove.GetCompEM().IsWorking();

            parent = parent.GetHierarchyParent();
        }

        return false;
    }

    ItemBase GetCollectionBottle()
    {
        return ItemBase.Cast(FindAttachmentBySlotName("StillBottle"));
    }

    ItemBase GetCondenserPipe()
    {
        return ItemBase.Cast(FindAttachmentBySlotName("StillPipe"));
    }

    ItemBase GetThermometer()
    {
        return ItemBase.Cast(FindAttachmentBySlotName("StillThermometer"));
    }

    bool HasCollectionBottle()
    {
        return GetCollectionBottle() != null;
    }

    // Moves the pipe's temperature part of the way up to target (never down;
    // it cools off on its own).
    protected void WarmPipeToward(float target, float rate)
    {
        ItemBase pipe = GetCondenserPipe();
        if (!pipe || !pipe.CanHaveTemperature() || pipe.GetTemperature() >= target)
            return;

        float temperature = pipe.GetTemperature();
        pipe.SetTemperatureDirect(Math.Min(temperature + (target - temperature) * rate + 0.5, target));
    }

    bool HasCondenserPipe()
    {
        return GetCondenserPipe() != null;
    }

    protected void UpdateStill()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        m_TickHeated = false;
        m_TickMash = false;
        StillTick(m_TickMilliseconds / 1000.0);

        int next = 0;
        if (GetQuantity() > 0 && !IsRuined())
        {
            if (m_TickHeated)
                next = STILL_TICK_FAST_MS;
            else if (m_TickMash)
                next = STILL_TICK_SLOW_MS;
        }
        SetStillTick(next);
    }

    // One tick; dt is the seconds since the last one.
    protected void StillTick(float dt)
    {
        // Empty: clear any batch state.
        if (GetQuantity() <= 0)
        {
            m_StillProgress = 0.0;
            m_LegacyBatch = STILL_LEGACY_NONE;
            UpdateFermentation(false, dt);
            return;
        }

        // A ruined still neither ferments nor distills (vanilla stops cooking
        // in ruined pots too); its contents stay as they are.
        if (IsRuined())
        {
            m_StillProgress = 0.0;
            return;
        }

        // A batch saved by an older version becomes beer now.
        if (m_LegacyBatch != STILL_LEGACY_NONE)
        {
            if (HasMash())
                ConvertToBeer(m_LegacyBatch == STILL_LEGACY_FERMENTING);
            m_LegacyBatch = STILL_LEGACY_NONE;
        }

        bool heated = HasHeatSource();
        m_TickHeated = heated;

        UpdateFermentation(heated, dt);

        // The thermometer's probe sits in the liquid, so it reads (and has)
        // the liquid's temperature, which cannot pass 100 °C.
        float reading = GetTemperature();
        if (GetQuantity() > 0)
            reading = Math.Min(reading, 100.0);
        ItemBase thermometer = GetThermometer();
        if (thermometer && thermometer.CanHaveTemperature())
            thermometer.SetTemperatureDirect(reading);

        // The pipe end touching the hot pot warms a little by conduction.
        WarmPipeToward(Math.Min(reading, STILL_PIPE_CONDUCTION_TEMPERATURE), 0.1);

        if (!heated || !HasCondenserPipe() || !HasCollectionBottle())
        {
            m_StillProgress = 0.0;
            return;
        }

        ItemBase bottle = GetCollectionBottle();
        if (!bottle || !bottle.IsLiquidContainer() || bottle.GetQuantityMax() <= bottle.GetQuantity())
        {
            m_StillProgress = 0.0;
            return;
        }

        StillSettings s = StillSettings.Get();
        int outputType = LIQUID_NONE;
        float inputPerOutput = 1.0; // ml of the still's liquid used per ml distilled
        int inputLiquidType = GetLiquidType();

        // Beer distills to the spirit. Any water (salt, pond, river, snow,
        // or a mash that has not fermented yet) distills to vanilla water with
        // no disease, so it mixes with any other water like vanilla's does.
        if (inputLiquidType == LIQUID_BEER)
        {
            outputType = LIQUID_VODKA;
            inputPerOutput = Math.Max(s.BeerPerSpiritMl, 1.0);
        }
        else if (inputLiquidType & LIQUID_GROUP_WATER)
        {
            outputType = LIQUID_WATER;
        }

        // Clean Water from older versions is water too.
        int bottleLiquid = bottle.GetLiquidType();
        if (bottleLiquid == LIQUID_CLEANWATER)
            bottleLiquid = LIQUID_WATER;

        if (outputType == LIQUID_NONE || (bottle.GetQuantity() > 0 && bottleLiquid != outputType))
        {
            m_StillProgress = 0.0;
            return;
        }

        // Nothing comes over until the liquid is hot enough: alcohol from
        // about 78 °C, water at its boiling point.
        float startTemperature = 100.0;
        if (Liquid.m_LiquidInfosByType.Contains(inputLiquidType))
            startTemperature = Math.Min(Liquid.GetBoilThreshold(inputLiquidType), 100.0);
        if (outputType == LIQUID_VODKA)
            startTemperature = Math.Min(startTemperature, s.MashDistillTemperature);
        if (GetTemperature() < startTemperature)
        {
            m_StillProgress = 0.0;
            return;
        }

        // Steam is going through the pipe: it heats up (it cools off on its
        // own once the still stops).
        WarmPipeToward(STILL_PIPE_TEMPERATURE, 0.25);

        m_StillProgress += dt;
        if (m_StillProgress < STILL_BATCH_SECONDS)
            return;

        // batchQuantity is the distillate before the thermometer bonus; the
        // still's liquid used is batchQuantity * inputPerOutput.
        float yieldFactor = 1.0;
        if (outputType == LIQUID_VODKA)
            yieldFactor = GetVodkaYieldFactor();

        float availableOutput = bottle.GetQuantityMax() - bottle.GetQuantity();
        float batchQuantity = Math.Min(STILL_BATCH_QUANTITY, availableOutput / yieldFactor);
        batchQuantity = Math.Min(batchQuantity, GetQuantity() / inputPerOutput);

        if (batchQuantity <= 0)
        {
            m_StillProgress = 0.0;
            return;
        }

        // The distillate drips in warm (an air-cooled pipe); mix it with what
        // the bottle holds. The bottle cools off again once the still stops.
        float before = bottle.GetQuantity();
        float added = batchQuantity * yieldFactor;
        if (bottle.CanHaveTemperature())
            bottle.SetTemperatureDirect((bottle.GetTemperature() * before + STILL_DISTILLATE_TEMPERATURE * added) / (before + added));

        // Distillate carries no disease: an empty vessel starts clean, while
        // water already in it keeps whatever it had (clean into dirty is dirty).
        if (before <= 0)
            bottle.RemoveAllAgents();
        bottle.SetLiquidType(outputType);
        bottle.SetQuantity(before + added);
        SetQuantity(GetQuantity() - batchQuantity * inputPerOutput);

        if (bottle.IsKindOf("WaterBottle"))
        {
            bottle.DecreaseHealth(1.0);
        }

        m_StillProgress = 0.0;
    }

    // Fresh water with enough fruit or potatoes ferments while unheated; heat
    // pauses it, and its speed follows the liquid temperature. Salt water
    // never ferments (salt kills yeast). When it finishes the water becomes
    // beer. Removing the fruit or the water resets it.
    protected void UpdateFermentation(bool heated, float dt)
    {
        StillSettings s = StillSettings.Get();
        float needed = s.FermentationMinutes * 60.0;
        int state = STILL_FERMENT_NONE;
        int pause = STILL_PAUSE_NONE;
        bool fermenting = false;

        if (IsFreshWater())
        {
            float capacity = GetFermentCapacity();
            if (capacity > 0.0)
            {
                fermenting = true;
                state = STILL_FERMENT_FERMENTING;

                if (capacity + 0.5 < GetQuantity())
                {
                    pause = STILL_PAUSE_FRUIT; // keeps its progress until there is enough
                }
                else if (needed <= 0.0)
                {
                    m_FermentSeconds = needed;
                }
                else if (!heated)
                {
                    m_FermentSeconds += dt * GetFermentRate(s, pause);
                }

                if (pause != STILL_PAUSE_FRUIT && m_FermentSeconds >= needed)
                {
                    ConvertToBeer(true);
                    fermenting = false;
                    state = STILL_FERMENT_NONE;
                    pause = STILL_PAUSE_NONE;
                }
            }
        }

        if (!fermenting)
            m_FermentSeconds = 0.0;
        m_TickMash = fermenting;

        if (state != m_FermentState || pause != m_FermentPause)
        {
            m_FermentState = state;
            m_FermentPause = pause;
            SetSynchDirty();
        }
    }

    // The fermented water becomes beer. Fruit is used in cargo order only as
    // far as the water needs; a partly needed item is partly used. Fermenting
    // gives each germ already in the water a chance to die, then rolls for a
    // bad batch (food poisoning travels with the beer). A batch saved by an
    // older version that had already been rolled skips both.
    protected void ConvertToBeer(bool fresh)
    {
        StillSettings s = StillSettings.Get();
        float volumeLeft = GetQuantity();
        bool rotten = false;

        array<Edible_Base> fruit = new array<Edible_Base>;
        CargoBase cargo = GetInventory().GetCargo();
        if (cargo)
        {
            for (int i = 0; i < cargo.GetItemCount(); i++)
            {
                Edible_Base edible = Edible_Base.Cast(cargo.GetItem(i));
                if (IsFermentable(edible))
                    fruit.Insert(edible);
            }
        }

        foreach (Edible_Base item : fruit)
        {
            if (volumeLeft <= 0.0)
                break;

            float itemCapacity = GetFruitCapacity(item);
            if (itemCapacity <= 0.0)
                continue;

            if (item.IsFoodRotten())
                rotten = true;

            if (itemCapacity <= volumeLeft + 0.01 || !item.HasQuantity())
            {
                volumeLeft -= itemCapacity;
                item.Delete();
            }
            else
            {
                item.SetQuantity(item.GetQuantity() * (1.0 - volumeLeft / itemCapacity));
                volumeLeft = 0.0;
            }
        }

        if (fresh)
        {
            int agents = GetAgents();
            for (int bit = 0; bit < 31; bit++)
            {
                int agent = 1 << bit;
                if ((agents & agent) && Math.RandomFloat01() < s.FermentKillsGermsChance)
                    RemoveAgent(agent);
            }

            float badChance = s.MashFoodPoisonChance;
            if (rotten)
                badChance = s.RottenMashFoodPoisonChance;
            if (Math.RandomFloat01() < badChance)
                InsertAgent(eAgents.FOOD_POISON, 1);
        }

        SetLiquidType(LIQUID_BEER);
        m_FermentSeconds = 0.0;
    }

    // Fruit or potatoes, cooked, dried or rotten (vanilla's "fruit" includes
    // berries and vegetables). Burnt ones have no sugar left to ferment,
    // ruined ones are useless as in vanilla, and cannabis has no sugar.
    protected bool IsFermentable(Edible_Base edible)
    {
        return edible && (edible.IsFruit() || edible.IsKindOf("Potato")) && !edible.IsKindOf("Cannabis") && !edible.IsFoodBurned() && !edible.IsRuined();
    }

    // Vegetables with little sugar make a weak mash.
    protected bool IsLowSugar(ItemBase item)
    {
        return item.IsKindOf("Tomato") || item.IsKindOf("GreenBellPepper") || item.IsKindOf("Zucchini");
    }

    // ml of water one fruit/potato can ferment: WaterPerFruitSlot per inventory
    // slot it takes up, less for rotten or low-sugar ones and for what has
    // been eaten.
    protected float GetFruitCapacity(ItemBase item)
    {
        if (!item || !g_Game)
            return 0.0;

        StillSettings s = StillSettings.Get();
        TIntArray itemSize = new TIntArray;
        g_Game.ConfigGetIntArray("CfgVehicles " + item.GetType() + " itemSize", itemSize);
        int cargoArea = 1;
        if (itemSize.Count() >= 2)
            cargoArea = Math.Max(itemSize[0] * itemSize[1], 1);

        float capacity = cargoArea * Math.Max(s.WaterPerFruitSlot, 1.0);
        Edible_Base edible = Edible_Base.Cast(item);
        if (edible && edible.IsFoodRotten())
            capacity *= s.RottenFruitVodkaYield;
        if (IsLowSugar(item))
            capacity *= s.LowSugarVegetableVodkaYield;

        if (item.HasQuantity() && item.GetQuantityMax() > 0)
            capacity *= Math.Clamp(item.GetQuantity() / item.GetQuantityMax(), 0.0, 1.0);

        return Math.Max(capacity, 0.0);
    }

    // ml of water all the fruit/potatoes in the still can ferment.
    protected float GetFermentCapacity()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return 0.0;

        float capacity = 0.0;
        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            Edible_Base edible = Edible_Base.Cast(cargo.GetItem(i));
            if (IsFermentable(edible))
                capacity += GetFruitCapacity(edible);
        }

        return capacity;
    }

    protected ItemBase FindFermentableInput()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return null;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            Edible_Base edible = Edible_Base.Cast(cargo.GetItem(i));
            if (IsFermentable(edible))
                return edible;
        }

        return null;
    }

    // Temperature control with a working thermometer improves the yield of
    // each spirit batch made while it is attached.
    protected float GetVodkaYieldFactor()
    {
        ItemBase thermometer = GetThermometer();
        if (thermometer && !thermometer.IsRuined())
            return 1.0 + StillSettings.Get().ThermometerVodkaBonus;

        return 1.0;
    }
};
