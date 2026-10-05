// Vanilla radiators accept only LIQUID_WATER. Distilled Clean Water is at
// least as good, so it passes the coolant check too (salt water stays
// blocked). Cars store coolant as an amount, not a liquid type.
modded class ActionFillCoolant
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        // Mash (water with fruit or potatoes in a still) is not coolant.
        ImprovisedStill still = ImprovisedStill.Cast(item);
        if (still && still.HasMash())
            return false;

        // Only the container being checked reports Clean Water as Water, and
        // only for the duration of the vanilla check.
        Bottle_Base bottle = Bottle_Base.Cast(item);
        if (bottle)
            bottle.StillSetCoolantCheck(true);

        bool result = super.ActionCondition(player, target, item);

        if (bottle)
            bottle.StillSetCoolantCheck(false);
        return result;
    }
}

modded class Bottle_Base
{
    protected bool m_StillCoolantCheck;

    void StillSetCoolantCheck(bool checking)
    {
        m_StillCoolantCheck = checking;
    }

    override int GetLiquidType()
    {
        int liquid = super.GetLiquidType();
        if (m_StillCoolantCheck && liquid == LIQUID_CLEANWATER)
            return LIQUID_WATER;

        return liquid;
    }
}
