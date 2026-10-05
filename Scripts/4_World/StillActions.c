// DayZ's default container-fill mask excludes salt water. This action lets
// every container that vanilla can fill with water (Bottle_Base: bottles,
// canteens, pouches, pots, cauldrons, jerry cans, the still) fill from the sea.
class ActionFillStillSaltWater : ActionFillBottleBase
{
    void ActionFillStillSaltWater()
    {
        m_AllowedLiquidMask = LIQUID_SALTWATER;
    }
}

modded class Bottle_Base
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionFillStillSaltWater);
    }
}

// Custom actions must be part of the global action registry before an item
// can add them from SetActions().
modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        actions.Insert(ActionFillStillSaltWater);
        actions.Insert(ActionDisinfectSelfVodka);
        actions.Insert(ActionDisinfectTargetVodka);
    }
}
