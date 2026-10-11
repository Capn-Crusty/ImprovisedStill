// Vanilla waters garden slots only with plain water, but its plant watering
// action checks no liquid at all. Salt water (which this mod adds) kills
// plants, so refuse it there too; other liquids are left to vanilla.
modded class ActionWaterPlant
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (item && item.GetLiquidType() == LIQUID_SALTWATER)
            return false;

        return super.ActionCondition(player, target, item);
    }
}
