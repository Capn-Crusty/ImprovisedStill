// Intoxication from drinking vodka and beer. Alcohol is absorbed as the
// stomach digests it, so food in the stomach slows it down (vanilla shares
// digestion between everything in the stomach). The level wears off over
// time. It is stored as a hidden agent so vanilla's agent persistence saves
// it with the character (no change to the player's save format).
const int STILL_MDF_INTOXICATION = 1001;   // above vanilla eModifiers.COUNT
const int STILL_AGENT_ALCOHOL = 2097152;   // 1 << 21, unused by vanilla eAgents

// Inert carrier for the intoxication level: never grows, dies off, spreads
// or reacts to medicine; only this mod changes it.
class StillAlcoholAgent extends AgentBase
{
    override void Init()
    {
        m_Type                  = STILL_AGENT_ALCOHOL;
        m_Invasibility          = 0;
        m_DieOffSpeed           = 0;
        m_Digestibility         = 0;
        m_TransferabilityIn     = 0;
        m_TransferabilityOut    = 0;
        m_AntibioticsResistance = 1;
        m_MaxCount              = 5000;
        m_Potency               = EStatLevels.GREAT;
        m_DrugResistances.Set(EMedicalDrugsType.ANTIBIOTICS, 1.0);
        m_DrugResistances.Set(EMedicalDrugsType.CHELATION, 1.0);
    }
}

const int STILL_DRUNK_SOBER = 0;
const int STILL_DRUNK_TIPSY = 1;
const int STILL_DRUNK_DRUNK = 2;
const int STILL_DRUNK_VERY = 3;
const int STILL_DRUNK_BLACKOUT = 4;

modded class PlayerBase
{
    protected int m_StillDrunkTier;     // synced to the client for visual effects

    void PlayerBase()
    {
        RegisterNetSyncVariableInt("m_StillDrunkTier", 0, 4);
    }

    // Server only (the agent pool lives on the server).
    float StillGetAlcohol()
    {
        if (!m_AgentPool || !m_AgentPool.m_VirusPool)
            return 0;
        return m_AgentPool.m_VirusPool.Get(STILL_AGENT_ALCOHOL);
    }

    void StillAddAlcohol(float units)
    {
        if (!m_AgentPool)
            return;
        m_AgentPool.SetAgentCount(STILL_AGENT_ALCOHOL, Math.Max(0, StillGetAlcohol() + units));
    }

    int StillGetDrunkTier()
    {
        return m_StillDrunkTier;
    }

    void StillSetDrunkTier(int tier)
    {
        if (tier == m_StillDrunkTier)
            return;

        m_StillDrunkTier = tier;
        SetSynchDirty();
    }
}

// The stomach item being digested does not know its player, so the stomach
// names the player for the duration of its digestion pass.
modded class PlayerStomach
{
    static PlayerBase s_StillDigestingPlayer;

    override void ProcessNutrients(float delta_time)
    {
        s_StillDigestingPlayer = m_Player;
        super.ProcessNutrients(delta_time);
        s_StillDigestingPlayer = null;
    }
}

modded class StomachItem
{
    override bool ProcessDigestion(float digestion_points, out float water, out float energy, out float toxicity, out float volume, out int agents, out float consumed_amount)
    {
        bool done = super.ProcessDigestion(digestion_points, water, energy, toxicity, volume, agents, consumed_amount);

        PlayerBase player = PlayerStomach.s_StillDigestingPlayer;
        if (player && consumed_amount > 0)
        {
            StillSettings s = StillSettings.Get();
            if (m_ClassName == "Vodka")
                player.StillAddAlcohol(consumed_amount * s.VodkaAlcoholPerMl);
            else if (m_ClassName == "Beer")
                player.StillAddAlcohol(consumed_amount * s.BeerAlcoholPerMl);
        }

        return done;
    }
}

class StillIntoxicationMdfr : ModifierBase
{
    protected bool m_BlackedOut;

    override void Init()
    {
        m_TrackActivatedTime    = false;
        m_ID                    = STILL_MDF_INTOXICATION;
        m_TickIntervalInactive  = DEFAULT_TICK_TIME_INACTIVE;
        m_TickIntervalActive    = DEFAULT_TICK_TIME_ACTIVE;
    }

    override protected bool ActivateCondition(PlayerBase player)
    {
        return player.StillGetAlcohol() > 0;
    }

    override protected bool DeactivateCondition(PlayerBase player)
    {
        return player.StillGetAlcohol() <= 0;
    }

    // Activation happens on the first drink (low level) or on login with a
    // saved level; never knock out a player just for logging in.
    override protected void OnActivate(PlayerBase player)
    {
        StillSettings s = StillSettings.Get();
        m_BlackedOut = s.BlackoutUnits > 0 && player.StillGetAlcohol() >= s.BlackoutUnits;
    }

    override protected void OnDeactivate(PlayerBase player)
    {
        player.StillSetDrunkTier(STILL_DRUNK_SOBER);
        m_BlackedOut = false;
    }

    override protected void OnTick(PlayerBase player, float deltaT)
    {
        StillSettings s = StillSettings.Get();
        player.StillAddAlcohol(-s.SoberingUnitsPerMinute / 60.0 * deltaT);

        float units = player.StillGetAlcohol();
        int tier = STILL_DRUNK_SOBER;
        if (s.BlackoutUnits > 0 && units >= s.BlackoutUnits)
            tier = STILL_DRUNK_BLACKOUT;
        else if (units >= s.VeryDrunkUnits)
            tier = STILL_DRUNK_VERY;
        else if (units >= s.DrunkUnits)
            tier = STILL_DRUNK_DRUNK;
        else if (units >= s.TipsyUnits)
            tier = STILL_DRUNK_TIPSY;
        player.StillSetDrunkTier(tier);

        // Alcohol dehydrates.
        player.GetStatWater().Add(-units * s.AlcoholWaterLossPerUnit * deltaT);

        if (tier >= STILL_DRUNK_VERY && Math.RandomFloat01() < s.VeryDrunkVomitChance * deltaT / 3.0)
            StillQueueVomit(player);

        // Pass out once per blackout; vanilla shock recovery wakes the player.
        if (tier == STILL_DRUNK_BLACKOUT && !m_BlackedOut)
        {
            m_BlackedOut = true;
            player.SetHealth("", "Shock", 0);
        }
        else if (tier < STILL_DRUNK_BLACKOUT)
        {
            m_BlackedOut = false;
        }

        // Pain relief: vanilla painkiller effect (halves shock damage, stops
        // pain limping), kept on while drunk enough.
        if (s.PainReliefFromTier > 0 && tier >= s.PainReliefFromTier && !player.GetModifiersManager().IsModifierActive(eModifiers.MDF_PAINKILLERS))
            player.GetModifiersManager().ActivateModifier(eModifiers.MDF_PAINKILLERS);

        // The warm feeling of a drink.
        if (tier >= STILL_DRUNK_TIPSY && s.AlcoholWarmthPerSecond > 0)
            player.GetStatHeatBuffer().Add(s.AlcoholWarmthPerSecond * deltaT);
    }
}
