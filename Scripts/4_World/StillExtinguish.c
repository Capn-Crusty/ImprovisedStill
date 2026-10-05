// Vanilla puts out fires only with plain water, blood or beer. This mod's
// salt water and clean water are separate liquid types, so allow them too.
modded class ActionExtinguishFireplaceByLiquid
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (super.ActionCondition(player, target, item))
            return true;

        if (!item || !(item.GetLiquidType() & (LIQUID_SALTWATER | LIQUID_CLEANWATER)))
            return false;

        FireplaceBase fireplace = FireplaceBase.Cast(target.GetObject());
        return fireplace && fireplace.CanExtinguishFire() && !item.IsDamageDestroyed() && !item.GetIsFrozen();
    }
}
