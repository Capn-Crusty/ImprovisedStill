// The name players see for the still's spirit (vanilla's vodka liquid),
// set by SpiritName in the server settings. Settings live on the server, so
// it sends the name to each player when they connect.
const int STILL_RPC_SPIRIT_NAME = 38130558; // unique to this mod

class StillSpiritName
{
    static const string DEFAULT_NAME = "Moonshine";
    protected static string s_ClientName;

    static string Get()
    {
        string name;
        if (g_Game && g_Game.IsServer())
            name = StillSettings.Get().SpiritName;
        else
            name = s_ClientName;

        name = name.Trim();
        if (name == "" || name.Length() > 32)
            return DEFAULT_NAME;
        return name;
    }

    static void SendTo(PlayerBase player, PlayerIdentity identity)
    {
        if (!player || !identity)
            return;
        g_Game.RPCSingleParam(player, STILL_RPC_SPIRIT_NAME, new Param1<string>(Get()), true, identity);
    }

    static void Receive(ParamsReadContext ctx)
    {
        Param1<string> data = new Param1<string>("");
        if (ctx.Read(data))
            s_ClientName = data.param1;
    }
}

modded class PlayerBase
{
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type == STILL_RPC_SPIRIT_NAME)
        {
            if (!g_Game.IsServer())
                StillSpiritName.Receive(ctx);
            return;
        }

        super.OnRPC(sender, rpc_type, ctx);
    }
}
