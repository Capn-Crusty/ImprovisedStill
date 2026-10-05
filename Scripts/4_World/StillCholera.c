// Saltwater sickness: a cholera-like disease only from drinking salt water
// (SaltWater "agents" in Data/config.cpp must match STILL_AGENT_SALTWATER).
// Antibiotics do not cure it; a completed saline IV does. Real cholera is
// left to vanilla (tetracycline).
const int STILL_AGENT_SALTWATER = 1048576; // 1 << 20, unused by vanilla eAgents
const int STILL_MDF_SALTWATER = 1000;      // above vanilla eModifiers.COUNT

class StillSaltwaterAgent extends AgentBase
{
    override void Init()
    {
        m_Type                  = STILL_AGENT_SALTWATER;
        StillSettings s = StillSettings.Get();
        m_Invasibility          = s.SaltwaterAgentGrowth;
        m_Digestibility         = s.SaltwaterAgentsPerMl;
        m_TransferabilityIn     = 0.1;
        m_TransferabilityOut    = 0;
        m_AntibioticsResistance = 1;
        m_MaxCount              = 1000;
        m_Potency               = EStatLevels.HIGH;
        m_DieOffSpeed           = s.SaltwaterAgentDieOff;
        m_DrugResistances.Set(EMedicalDrugsType.ANTIBIOTICS, 1.0);
    }
}

modded class PluginTransmissionAgents
{
    void PluginTransmissionAgents()
    {
        RegisterAgent(new StillSaltwaterAgent);
        RegisterAgent(new StillAlcoholAgent);
    }
}

// Same symptoms as vanilla CholeraMdfr, driven by the
// saltwater agent.
class StillSaltwaterSicknessMdfr : ModifierBase
{
    static const float VOMIT_COOLDOWN = 15;
    static const int CHANCE_OF_VOMIT = 10;
    static const int CHANCE_OF_VOMIT_AGENT = 30;
    static const int WATER_DRAIN_FROM_VOMIT = 450;
    static const int ENERGY_DRAIN_FROM_VOMIT = 310;
    static const float WATER_LOSS = 0.5;
    static const float WATER_LOSS_MIN = 0.1;
    static const float STOMACH_MIN_VOLUME = 200;

    protected float m_ExhaustionTimer;
    protected bool m_Exhaustion;
    protected float m_LastSaltVomitTime = -1000;

    override void Init()
    {
        m_TrackActivatedTime    = false;
        m_ID                    = STILL_MDF_SALTWATER;
        m_TickIntervalInactive  = DEFAULT_TICK_TIME_INACTIVE;
        m_TickIntervalActive    = DEFAULT_TICK_TIME_ACTIVE;
    }

    override protected bool ActivateCondition(PlayerBase player)
    {
        CheckSaltwaterVomit(player);
        return player.GetSingleAgentCount(STILL_AGENT_SALTWATER) >= StillSettings.Get().SaltwaterSicknessStartAgents;
    }

    override protected bool DeactivateCondition(PlayerBase player)
    {
        return player.GetSingleAgentCount(STILL_AGENT_SALTWATER) <= StillSettings.Get().SaltwaterSicknessEndAgents;
    }

    override protected void OnActivate(PlayerBase player)
    {
        player.IncreaseDiseaseCount();
        StillQueueVomit(player);
    }

    override protected void OnDeactivate(PlayerBase player)
    {
        player.GetStaminaHandler().DeactivateRecoveryModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
        player.GetStaminaHandler().DeactivateDepletionModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
        m_Exhaustion = false;
        m_ExhaustionTimer = 0;
        player.DecreaseDiseaseCount();
    }

    override protected void OnTick(PlayerBase player, float deltaT)
    {
        CheckSaltwaterVomit(player);

        float severity = player.GetSingleAgentCountNormalized(STILL_AGENT_SALTWATER);
        player.GetStatWater().Add(-deltaT * WATER_LOSS * Math.Max(WATER_LOSS_MIN, severity));

        if (player.m_PlayerStomach.GetStomachVolume() >= STOMACH_MIN_VOLUME && Math.RandomInt(0, 100) < CHANCE_OF_VOMIT + CHANCE_OF_VOMIT_AGENT * severity)
        {
            if (StillQueueVomit(player))
            {
                if (player.GetStatWater().Get() > WATER_DRAIN_FROM_VOMIT)
                    player.GetStatWater().Add(-WATER_DRAIN_FROM_VOMIT);
                if (player.GetStatEnergy().Get() > ENERGY_DRAIN_FROM_VOMIT)
                    player.GetStatEnergy().Add(-ENERGY_DRAIN_FROM_VOMIT);

                player.GetStaminaHandler().ActivateRecoveryModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
                player.GetStaminaHandler().ActivateDepletionModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
                m_Exhaustion = true;
                m_ExhaustionTimer = 0;
            }
        }

        if (m_Exhaustion)
        {
            m_ExhaustionTimer += deltaT;
            if (m_ExhaustionTimer >= 30)
            {
                player.GetStaminaHandler().DeactivateRecoveryModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
                player.GetStaminaHandler().DeactivateDepletionModifier(EStaminaMultiplierTypes.VOMIT_EXHAUSTION);
                m_Exhaustion = false;
            }
        }
    }

    // Too much salt water in the stomach makes the player vomit at once,
    // whether or not the sickness has taken hold yet.
    protected void CheckSaltwaterVomit(PlayerBase player)
    {
        float limit = StillSettings.Get().SaltwaterVomitStomachMl;
        if (limit <= 0 || !player.m_PlayerStomach || player.m_PlayerStomach.GetVolumeContainingAgent(STILL_AGENT_SALTWATER) < limit)
            return;

        float now = g_Game.GetTickTime();
        if (now - m_LastSaltVomitTime < VOMIT_COOLDOWN)
            return;

        if (StillQueueVomit(player))
            m_LastSaltVomitTime = now;
    }
}

// Queues vanilla vomiting (empties 65% of the stomach). Shared by saltwater
// sickness and intoxication.
bool StillQueueVomit(PlayerBase player)
{
    SymptomBase symptom = player.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_VOMIT);
    if (!symptom)
        return false;

    CachedObjectsParams.PARAM1_FLOAT.param1 = 65.0;
    symptom.SetParam(CachedObjectsParams.PARAM1_FLOAT);
    symptom.SetDuration(Math.RandomIntInclusive(4.0, 8.0));
    return true;
}

modded class ModifiersManager
{
    override void Init()
    {
        super.Init();
        AddModifier(new StillSaltwaterSicknessMdfr);
        AddModifier(new StillIntoxicationMdfr);
    }
}

void StillCureSaltwaterSickness(PlayerBase player)
{
    if (!player)
        return;

    int count = player.GetSingleAgentCount(STILL_AGENT_SALTWATER);
    if (count <= 0)
        return;

    player.RemoveAgent(STILL_AGENT_SALTWATER);
    Print("[ImprovisedStill] Saline IV cleared saltwater sickness (" + count + " agents).");

    float resistance = StillSettings.Get().SalineResistanceSeconds;
    if (resistance > 0 && player.m_AgentPool)
        player.m_AgentPool.SetTemporaryResistance(STILL_AGENT_SALTWATER, resistance);
}

// Salt water dehydrates: each ml digested costs hydration (set in the server
// settings). Digested agent quantity equals the ml consumed because SaltWater
// has agentsPerDigest = 0.
modded class PlayerStomach
{
    override void DigestAgents(int agents, float quantity)
    {
        super.DigestAgents(agents, quantity);

        if ((agents & STILL_AGENT_SALTWATER) && quantity > 0)
            m_Player.GetStatWater().Add(-quantity * StillSettings.Get().SaltwaterHydrationLossPerMl);
    }
}

modded class ActionGiveSalineSelf
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        super.OnFinishProgressServer(action_data);
        StillCureSaltwaterSickness(action_data.m_Player);
    }
}

modded class ActionGiveSalineTarget
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        super.OnFinishProgressServer(action_data);
        StillCureSaltwaterSickness(PlayerBase.Cast(action_data.m_Target.GetObject()));
    }
}
