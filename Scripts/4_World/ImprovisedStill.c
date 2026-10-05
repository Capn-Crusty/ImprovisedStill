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
    protected const float STILL_TICK_SECONDS = 5.0;
    protected const float STILL_BATCH_SECONDS = 30.0;
    protected const float STILL_BATCH_QUANTITY = 50.0;
    protected const float STILL_ALCOHOL_PER_CARGO_SLOT = 100.0;
    protected const int STILL_TICK_MILLISECONDS = 5000;

    void ImprovisedStill()
    {
        m_StillProgress = 0.0;
        m_AlcoholOutputProduced = 0.0;
        m_AlcoholOutputTarget = 0.0;
        m_ActiveFermentableType = "";
        m_FermentSeconds = 0.0;
        m_FermentState = STILL_FERMENT_NONE;
        RegisterNetSyncVariableInt("m_FermentState", 0, 2);
    }

    int GetFermentState()
    {
        return m_FermentState;
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

    override void EEInit()
    {
        super.EEInit();

        if (g_Game && g_Game.IsServer())
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(UpdateStill, STILL_TICK_MILLISECONDS, true);
    }

    override void EEDelete(EntityAI parent)
    {
        if (g_Game && g_Game.IsServer())
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).Remove(UpdateStill);

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

        m_AlcoholOutputProduced = Math.Clamp(m_AlcoholOutputProduced, 0.0, 750.0);
        m_AlcoholOutputTarget = Math.Clamp(m_AlcoholOutputTarget, 0.0, 750.0);
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

    bool HasCondenserPipe()
    {
        return GetCondenserPipe() != null;
    }

    protected void UpdateStill()
    {
        if (!g_Game || !g_Game.IsServer())
            return;

        // Look these up once per tick; fermentation and distilling both need them.
        bool heated = HasHeatSource();
        ItemBase fermentable = FindFermentableInput();

        UpdateFermentation(heated, fermentable);

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
        // Any water (salt, pond, river, snow...) distills to clean water.
        else if (GetQuantity() > 0 && (inputLiquidType & LIQUID_GROUP_WATER))
            outputType = LIQUID_CLEANWATER;

        if (outputType == LIQUID_NONE || (bottle.GetQuantity() > 0 && bottle.GetLiquidType() != outputType))
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

        m_StillProgress += STILL_TICK_SECONDS;
        if (m_StillProgress < STILL_BATCH_SECONDS)
            return;

        // Batch quantities are in base units (water used, fermentable used up);
        // the vodka poured into the bottle is scaled by the thermometer bonus.
        float yieldFactor = 1.0;
        if (outputType == LIQUID_VODKA)
            yieldFactor = GetVodkaYieldFactor();

        float availableOutput = bottle.GetQuantityMax() - bottle.GetQuantity();
        float batchQuantity = Math.Min(STILL_BATCH_QUANTITY, availableOutput / yieldFactor);
        batchQuantity = Math.Min(batchQuantity, GetQuantity());
        if (outputType == LIQUID_VODKA)
            batchQuantity = Math.Min(batchQuantity, m_AlcoholOutputTarget - m_AlcoholOutputProduced);

        if (batchQuantity <= 0)
        {
            m_StillProgress = 0.0;
            return;
        }

        bottle.SetLiquidType(outputType);
        bottle.SetQuantity(bottle.GetQuantity() + batchQuantity * yieldFactor);
        SetQuantity(GetQuantity() - batchQuantity);

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
    // it. Salt water never ferments (salt kills yeast). Removing the fruit or
    // the water resets it.
    protected void UpdateFermentation(bool heated, ItemBase fermentable)
    {
        StillSettings s = StillSettings.Get();
        float needed = s.FermentationMinutes * 60.0;
        int state = STILL_FERMENT_NONE;
        int liquid = GetLiquidType();
        bool freshWater = (liquid & LIQUID_GROUP_WATER) && liquid != LIQUID_SALTWATER;

        if (!fermentable || GetQuantity() <= 0 || !freshWater)
        {
            // A new batch starts clean.
            if (m_FermentSeconds > 0.0)
                RemoveAgent(eAgents.FOOD_POISON);
            m_FermentSeconds = 0.0;
        }
        else
        {
            // m_FermentSeconds is saved, so crossing into "ready" happens once
            // per batch and the food-poisoning roll is not repeated on reload.
            // With instant fermentation the first tick of a batch counts.
            bool becameReady = false;
            if (needed <= 0.0)
            {
                if (m_FermentSeconds <= 0.0)
                {
                    m_FermentSeconds = STILL_TICK_SECONDS;
                    becameReady = true;
                }
            }
            else if (m_FermentSeconds < needed && !heated)
            {
                m_FermentSeconds += STILL_TICK_SECONDS;
                becameReady = m_FermentSeconds >= needed;
            }

            if (becameReady && Math.RandomFloat01() < s.MashFoodPoisonChance)
                InsertAgent(eAgents.FOOD_POISON, 1);

            if (m_FermentSeconds >= needed)
                state = STILL_FERMENT_READY;
            else
                state = STILL_FERMENT_FERMENTING;
        }

        if (state != m_FermentState)
        {
            m_FermentState = state;
            SetSynchDirty();
        }
    }

    protected ItemBase FindFermentableInput()
    {
        CargoBase cargo = GetInventory().GetCargo();
        if (!cargo)
            return null;

        for (int i = 0; i < cargo.GetItemCount(); i++)
        {
            ItemBase item = ItemBase.Cast(cargo.GetItem(i));
            Edible_Base edible = Edible_Base.Cast(item);
            if (edible && (edible.IsFruit() || edible.IsKindOf("Potato")))
                return edible;
        }

        return null;
    }

    protected float GetFermentableOutputTarget(ItemBase item)
    {
        if (!item || !g_Game)
            return STILL_ALCOHOL_PER_CARGO_SLOT;

        TIntArray itemSize = new TIntArray;
        g_Game.ConfigGetIntArray("CfgVehicles " + item.GetType() + " itemSize", itemSize);
        if (itemSize.Count() < 2)
            return STILL_ALCOHOL_PER_CARGO_SLOT;

        int cargoArea = itemSize[0] * itemSize[1];
        if (cargoArea < 1)
            cargoArea = 1;

        return cargoArea * STILL_ALCOHOL_PER_CARGO_SLOT;
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
