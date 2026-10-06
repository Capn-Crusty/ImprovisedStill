class ImprovisedStill extends Pot
{
    protected float m_StillProgress;
    protected float m_AlcoholOutputProduced;
    protected float m_AlcoholOutputTarget;
    protected string m_ActiveFermentableType;
    protected float m_FermentSeconds;
    protected int m_FermentState; // STILL_FERMENT_*, synced for the tooltip
    static const int STILL_FERMENT_NONE = 0;
    static const int STILL_FERMENT_FERMENTING = 1;
    static const int STILL_FERMENT_READY = 2;
    static const int STILL_FERMENT_SPOILED = 3;
    protected int m_FermentableCount = -1; // fruit/potatoes seen last tick; -1 until the first tick after load
    protected int m_FermentPause; // STILL_PAUSE_*, why fermentation is stalled, synced for the tooltip
    static const int STILL_PAUSE_NONE = 0;
    static const int STILL_PAUSE_COLD = 1;
    static const int STILL_PAUSE_HOT = 2;
    protected const float STILL_INSTANT_FERMENT_SECONDS = 5.0; // marks a batch as started with instant fermentation
    protected const float STILL_BATCH_SECONDS = 30.0;
    protected const float STILL_BATCH_QUANTITY = 50.0;
    protected const float STILL_DISTILLATE_TEMPERATURE = 35.0;
    protected const float STILL_PIPE_TEMPERATURE = 60.0;            // with steam going through it
    protected const float STILL_PIPE_CONDUCTION_TEMPERATURE = 40.0; // from the hot pot alone
    protected const int STILL_TICK_FAST_MS = 5000;  // heated: distilling needs it
    protected const int STILL_TICK_SLOW_MS = 30000; // fermenting or ready mash (minutes-long timers)

    void ImprovisedStill()
    {
        m_StillProgress = 0.0;
        m_AlcoholOutputProduced = 0.0;
        m_AlcoholOutputTarget = 0.0;
        m_ActiveFermentableType = "";
        m_FermentSeconds = 0.0;
        m_FermentState = STILL_FERMENT_NONE;
        RegisterNetSyncVariableInt("m_FermentState", 0, 3);
        RegisterNetSyncVariableInt("m_FermentPause", 0, 2);
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

    // Fruit or potatoes in the cargo make the liquid a mash.
    bool HasMash()
    {
        return FindFermentableInput() != null;
    }

    // Set by PlayerBase.Consume while a player drinks from this still, so
    // ready mash is digested as beer. Everything else sees the real liquid.
    protected bool m_DrinkingMash;

    void SetDrinkingMash(bool drinking)
    {
        m_DrinkingMash = drinking;
    }

    override int GetLiquidType()
    {
        if (m_DrinkingMash && m_FermentState == STILL_FERMENT_READY)
            return LIQUID_BEER;

        return super.GetLiquidType();
    }

    // To keep idle stills free on busy servers, a still only ticks when it has
    // work: every 5 s while heated, every 30 s while it holds mash, and not at
    // all when it is empty, ruined, spoiled or holds only water. Anything that
    // could give it work wakes it: filling it, adding or removing fruit,
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
    protected const float STILL_SAVE_VERSION_2 = -2.0; // adds m_FermentSeconds

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(STILL_SAVE_VERSION_2);
        ctx.Write(m_StillProgress);
        ctx.Write(m_AlcoholOutputProduced);
        ctx.Write(m_AlcoholOutputTarget);
        ctx.Write(m_ActiveFermentableType);
        ctx.Write(m_FermentSeconds);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        float first;
        if (!ctx.Read(first))
            return false;

        bool hasFermentation = first < 0.0;
        if (hasFermentation)
        {
            if (!ctx.Read(m_StillProgress))
                return false;
        }
        else
        {
            m_StillProgress = first; // original format
        }

        m_StillProgress = Math.Clamp(m_StillProgress, 0.0, STILL_BATCH_SECONDS);
        if (!ctx.Read(m_AlcoholOutputProduced) || !ctx.Read(m_AlcoholOutputTarget) || !ctx.Read(m_ActiveFermentableType))
            return false;

        m_FermentSeconds = 0.0;
        if (hasFermentation && !ctx.Read(m_FermentSeconds))
            return false;

        // Sanity limits only (a large item with a generous SpiritPerCargoSlot
        // can need several litres).
        m_AlcoholOutputProduced = Math.Clamp(m_AlcoholOutputProduced, 0.0, 100000.0);
        m_AlcoholOutputTarget = Math.Clamp(m_AlcoholOutputTarget, 0.0, 100000.0);
        return true;
    }

    bool HasHeatSource()
    {
        EntityAI parent = GetHierarchyParent();
        while (parent)
        {
            FireplaceBase fireplace = FireplaceBase.Cast(parent);
            if (fireplace)
                return fireplace.IsBurning();

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
            else if (m_TickMash && m_FermentState != STILL_FERMENT_SPOILED)
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
            UpdateFermentation(false, null, dt);
            return;
        }

        // A ruined still neither ferments nor distills (vanilla stops cooking
        // in ruined pots too); its contents stay as they are.
        if (IsRuined())
        {
            m_StillProgress = 0.0;
            return;
        }

        // Look these up once per tick; fermentation and distilling both need them.
        bool heated = HasHeatSource();
        ItemBase fermentable = FindFermentableInput();
        m_TickHeated = heated;
        m_TickMash = fermentable != null;

        UpdateFermentation(heated, fermentable, dt);

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

        int outputType = LIQUID_NONE;
        int inputLiquidType = GetLiquidType();

        // Unfermented mash distills as plain water.
        if (GetQuantity() > 0 && fermentable && (inputLiquidType & LIQUID_GROUP_WATER) && m_FermentState == STILL_FERMENT_READY)
        {
            outputType = LIQUID_VODKA;
            if (m_ActiveFermentableType != fermentable.GetType() || m_AlcoholOutputTarget <= 0.0)
            {
                m_ActiveFermentableType = fermentable.GetType();
                m_AlcoholOutputProduced = 0.0;
                m_AlcoholOutputTarget = GetFermentableOutputTarget(fermentable);
            }
        }
        // Any water (salt, pond, river, snow...) distills to vanilla water with
        // no disease, so it mixes with any other water like vanilla's does.
        else if (GetQuantity() > 0 && (inputLiquidType & LIQUID_GROUP_WATER))
            outputType = LIQUID_WATER;

        // Clean Water from older versions is water too.
        int bottleLiquid = bottle.GetLiquidType();
        if (bottleLiquid == LIQUID_CLEANWATER)
            bottleLiquid = LIQUID_WATER;

        if (outputType == LIQUID_NONE || (bottle.GetQuantity() > 0 && bottleLiquid != outputType))
        {
            m_StillProgress = 0.0;
            if (!fermentable && outputType != LIQUID_VODKA)
            {
                m_AlcoholOutputProduced = 0.0;
                m_AlcoholOutputTarget = 0.0;
                m_ActiveFermentableType = "";
            }
            return;
        }

        // Nothing comes over until the liquid is hot enough: alcohol from
        // about 78 °C, water at its boiling point.
        float startTemperature = 100.0;
        if (Liquid.m_LiquidInfosByType.Contains(inputLiquidType))
            startTemperature = Math.Min(Liquid.GetBoilThreshold(inputLiquidType), 100.0);
        if (outputType == LIQUID_VODKA)
            startTemperature = Math.Min(startTemperature, StillSettings.Get().MashDistillTemperature);
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

        // Batch quantities are in base units (fermentable used up); the vodka
        // poured into the bottle is scaled by the thermometer bonus, and the
        // mash water used up by MashWaterPerSpiritMl.
        float yieldFactor = 1.0;
        float waterPerMl = 1.0;
        if (outputType == LIQUID_VODKA)
        {
            yieldFactor = GetVodkaYieldFactor();
            waterPerMl = Math.Max(StillSettings.Get().MashWaterPerSpiritMl, 0.1);
        }

        float availableOutput = bottle.GetQuantityMax() - bottle.GetQuantity();
        float batchQuantity = Math.Min(STILL_BATCH_QUANTITY, availableOutput / yieldFactor);
        batchQuantity = Math.Min(batchQuantity, GetQuantity() / waterPerMl);
        if (outputType == LIQUID_VODKA)
            batchQuantity = Math.Min(batchQuantity, m_AlcoholOutputTarget - m_AlcoholOutputProduced);

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
        SetQuantity(GetQuantity() - batchQuantity * waterPerMl);

        if (outputType == LIQUID_VODKA && fermentable)
        {
            m_AlcoholOutputProduced += batchQuantity;
            if (m_AlcoholOutputProduced >= m_AlcoholOutputTarget)
            {
                fermentable.Delete();
                m_AlcoholOutputProduced = 0.0;
                m_AlcoholOutputTarget = 0.0;
                m_ActiveFermentableType = "";
            }
        }
        else
        {
            m_AlcoholOutputProduced = 0.0;
            m_AlcoholOutputTarget = 0.0;
            m_ActiveFermentableType = "";
        }

        if (bottle.IsKindOf("WaterBottle"))
        {
            bottle.DecreaseHealth(1.0);
        }

        m_StillProgress = 0.0;
    }

    // Fresh water with fruit or potatoes ferments while unheated; heat pauses
    // it, and its speed follows the liquid temperature. Salt water never
    // ferments (salt kills yeast). Ready mash left unheated too long spoils
    // (cold slows that too): its fruit rots and it gives no vodka. Removing
    // the fruit or the water resets it.
    protected void UpdateFermentation(bool heated, ItemBase fermentable, float dt)
    {
        StillSettings s = StillSettings.Get();
        float needed = s.FermentationMinutes * 60.0;
        bool canSpoil = s.MashSpoilMinutes > 0.0;
        float spoilAt = Math.Max(needed, 0.0) + s.MashSpoilMinutes * 60.0;
        int state = STILL_FERMENT_NONE;
        int pause = STILL_PAUSE_NONE;
        int liquid = GetLiquidType();
        bool freshWater = (liquid & LIQUID_GROUP_WATER) && liquid != LIQUID_SALTWATER;

        int fermentableCount = CountFermentables();
        bool addedFermentable = m_FermentableCount >= 0 && fermentableCount > m_FermentableCount;
        m_FermentableCount = fermentableCount;

        if (!fermentable || GetQuantity() <= 0 || !freshWater)
        {
            // A new batch starts clean.
            if (m_FermentSeconds > 0.0)
                RemoveAgent(eAgents.FOOD_POISON);
            m_FermentSeconds = 0.0;
        }
        else
        {
            // Fresh fruit added to ready mash has to ferment too, faster than
            // a new batch because the yeast is already working.
            if (addedFermentable && needed > 0.0 && m_FermentSeconds >= needed && !(canSpoil && m_FermentSeconds >= spoilAt))
                m_FermentSeconds = needed * (1.0 - Math.Clamp(s.TopUpFermentationFraction, 0.0, 1.0));

            // m_FermentSeconds is saved, so crossing into "ready" and "spoiled"
            // happens once per batch and the rolls are not repeated on reload.
            // With instant fermentation the first tick of a batch counts.
            bool becameReady = false;
            bool becameSpoiled = false;
            if (needed <= 0.0 && m_FermentSeconds <= 0.0)
            {
                m_FermentSeconds = STILL_INSTANT_FERMENT_SECONDS;
                becameReady = true;
            }
            else if (!heated && (m_FermentSeconds < needed || (canSpoil && m_FermentSeconds < spoilAt)))
            {
                float before = m_FermentSeconds;
                m_FermentSeconds += dt * GetFermentRate(s, pause);
                becameReady = before < needed && m_FermentSeconds >= needed;
                becameSpoiled = canSpoil && m_FermentSeconds >= spoilAt;
            }

            if (becameReady)
            {
                float badChance = s.MashFoodPoisonChance;
                if (HasRottenFermentable())
                    badChance = s.RottenMashFoodPoisonChance;
                if (Math.RandomFloat01() < badChance)
                    InsertAgent(eAgents.FOOD_POISON, 1);
            }

            if (becameSpoiled)
                SpoilMash();

            if (canSpoil && m_FermentSeconds >= spoilAt)
                state = STILL_FERMENT_SPOILED;
            else if (m_FermentSeconds >= needed)
                state = STILL_FERMENT_READY;
            else
                state = STILL_FERMENT_FERMENTING;
        }

        if (state != m_FermentState || pause != m_FermentPause)
        {
            m_FermentState = state;
            m_FermentPause = pause;
            SetSynchDirty();
        }
    }

    // Spoiled mash sours: the fruit in it rots and drinking it can give food
    // poisoning. Distilling it gives only clean water.
    protected void SpoilMash()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            Edible_Base edible = Edible_Base.Cast(cargo.GetItem(i));
            if (IsFermentable(edible) && !edible.IsFoodRotten())
                edible.ChangeFoodStage(FoodStageType.ROTTEN);
        }

        InsertAgent(eAgents.FOOD_POISON, 1);
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

    // Any rotten fruit or potato in the mash makes the whole batch more
    // likely to go bad.
    protected bool HasRottenFermentable()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return false;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            Edible_Base edible = Edible_Base.Cast(cargo.GetItem(i));
            if (IsFermentable(edible) && edible.IsFoodRotten())
                return true;
        }

        return false;
    }

    protected int CountFermentables()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return 0;

        int count = 0;
        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            if (IsFermentable(Edible_Base.Cast(cargo.GetItem(i))))
                count++;
        }

        return count;
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

    protected float GetFermentableOutputTarget(ItemBase item)
    {
        float perSlot = Math.Max(StillSettings.Get().SpiritPerCargoSlot, 1.0);
        if (!item || !g_Game)
            return perSlot;

        TIntArray itemSize = new TIntArray;
        g_Game.ConfigGetIntArray("CfgVehicles " + item.GetType() + " itemSize", itemSize);
        int cargoArea = 1;
        if (itemSize.Count() >= 2)
            cargoArea = Math.Max(itemSize[0] * itemSize[1], 1);

        float target = cargoArea * perSlot;
        Edible_Base edible = Edible_Base.Cast(item);
        if (edible && edible.IsFoodRotten())
            target *= StillSettings.Get().RottenFruitVodkaYield;
        if (IsLowSugar(item))
            target *= StillSettings.Get().LowSugarVegetableVodkaYield;

        // A partly eaten item gives its share.
        if (item.HasQuantity() && item.GetQuantityMax() > 0)
            target *= Math.Clamp(item.GetQuantity() / item.GetQuantityMax(), 0.0, 1.0);

        // Never zero, or the item would never be used up.
        return Math.Max(target, 1.0);
    }

    // Temperature control with a working thermometer improves the yield of
    // each vodka batch made while it is attached.
    protected float GetVodkaYieldFactor()
    {
        ItemBase thermometer = GetThermometer();
        if (thermometer && !thermometer.IsRuined())
            return 1.0 + StillSettings.Get().ThermometerVodkaBonus;

        return 1.0;
    }
};
