// Client only: dedicated servers do not have the post-process classes.
#ifndef SERVER
// Client-side drunk visuals: a pulsing blur on its own layer (so it never
// fights vanilla fever or pain blur) and occasional camera sway, scaled by
// the intoxication tier the server syncs to the player.
class PPERequester_StillDrunk extends PPERequester_GameplayBase
{
    static const int LAYER = 320; // between vanilla fever (300) and flashbang (400)

    // Vanilla validates a requester while constructing it, before a late
    // registration has put it in the bank, so re-check once registered.
    void StillRevalidate()
    {
        m_Valid = PPERequesterBank.VerifyRequester(this);
    }

    void SetDrunkBlur(float intensity)
    {
        SetTargetValueFloat(PostProcessEffectType.GaussFilter, PPEGaussFilter.PARAM_INTENSITY, true, intensity, LAYER, PPOperators.ADD_RELATIVE);
    }
}

modded class MissionGameplay
{
    protected float m_StillDrunkTime;
    protected float m_StillNextSway;
    protected PPERequester_StillDrunk m_StillDrunkRequester;

    // Blur base, pulse amplitude and sway strength per tier.
    protected void StillDrunkLook(int tier, out float blur, out float pulse, out float sway)
    {
        blur = 0; pulse = 0; sway = 0;
        if (tier == 1) { blur = 0.05; pulse = 0.03; }
        else if (tier == 2) { blur = 0.15; pulse = 0.08; sway = 0.15; }
        else if (tier >= 3) { blur = 0.3; pulse = 0.15; sway = 0.35; }
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        StillUpdateDrunkEffects(timeslice);
    }

    protected void StillUpdateDrunkEffects(float timeslice)
    {
        // Registered on first use through the public requester bank, then
        // kept so later frames skip the bank lookup.
        if (!m_StillDrunkRequester)
        {
            m_StillDrunkRequester = PPERequester_StillDrunk.Cast(PPERequesterBank.GetRequester(PPERequester_StillDrunk));
            if (!m_StillDrunkRequester)
            {
                PPERequesterBank.RegisterRequester(PPERequester_StillDrunk);
                m_StillDrunkRequester = PPERequester_StillDrunk.Cast(PPERequesterBank.GetRequester(PPERequester_StillDrunk));
                if (!m_StillDrunkRequester)
                    return;
                m_StillDrunkRequester.StillRevalidate();
            }
        }
        PPERequester_StillDrunk requester = m_StillDrunkRequester;

        PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
        int tier = 0;
        if (player && player.IsAlive())
            tier = Math.Clamp(player.StillGetDrunkTier(), 0, 4);

        if (tier == 0)
        {
            if (requester.IsRequesterRunning())
                requester.Stop();
            m_StillDrunkTime = 0;
            return;
        }

        if (!requester.IsRequesterRunning())
            requester.Start();

        float blur, pulse, sway;
        StillDrunkLook(tier, blur, pulse, sway);

        m_StillDrunkTime += timeslice;
        requester.SetDrunkBlur(Math.Max(0, blur + pulse * Math.Sin(m_StillDrunkTime * 0.8)));

        if (sway > 0)
        {
            m_StillNextSway -= timeslice;
            if (m_StillNextSway <= 0)
            {
                m_StillNextSway = Math.RandomFloatInclusive(3.0, 7.0);
                DayZPlayerCamera camera = player.GetCurrentCamera();
                if (camera)
                    camera.SpawnCameraShake(sway, 2, 8, 2);
            }
        }
    }
}
#endif
