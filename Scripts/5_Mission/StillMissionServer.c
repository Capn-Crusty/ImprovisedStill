// Sends the configured spirit name to each player as they connect, and again
// a few seconds later in case their character was not on their screen yet.
modded class MissionServer
{
    override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
    {
        super.InvokeOnConnect(player, identity);
        StillSendSpiritName(player, identity);
        g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(StillSendSpiritName, 5000, false, player, identity);
    }

    void StillSendSpiritName(PlayerBase player, PlayerIdentity identity)
    {
        StillSpiritName.SendTo(player, identity);
    }
}
