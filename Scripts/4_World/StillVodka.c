// Vodka in any bottle can disinfect wounds like alcohol tincture.
const float STILL_VODKA_DISINFECT_ML = 25.0;

// Drink containers only: not pots, cauldrons, the still or jerry cans.
bool StillIsDrinkContainer(ItemBase item)
{
    return item && (item.IsKindOf("GlassBottle") || item.IsKindOf("WaterBottle") || item.IsKindOf("Canteen") || item.IsKindOf("WaterPouch_ColorBase") || item.IsKindOf("FilteringBottle"));
}

bool StillHasVodkaForDisinfect(ItemBase item)
{
    return StillIsDrinkContainer(item) && item.GetLiquidType() == LIQUID_VODKA && item.GetQuantity() >= STILL_VODKA_DISINFECT_ML && !item.GetIsFrozen();
}

class ActionDisinfectSelfVodka : ActionDisinfectSelf
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        return StillHasVodkaForDisinfect(item) && super.ActionCondition(player, target, item);
    }
}

class ActionDisinfectTargetVodka : ActionDisinfectTarget
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        return StillHasVodkaForDisinfect(item) && super.ActionCondition(player, target, item);
    }
}

// Vodka disinfects items (rags, knives...) like tincture, from the same drink
// containers as above.
modded class DisinfectItem
{
    override void Init()
    {
        super.Init();
        InsertIngredient(0, "GlassBottle");
        InsertIngredient(0, "WaterBottle");
        InsertIngredient(0, "Canteen");
        InsertIngredient(0, "WaterPouch_ColorBase");
        InsertIngredient(0, "FilteringBottle");
    }

    override bool CanDo(ItemBase ingredients[], PlayerBase player)
    {
        if (ingredients[0] && ingredients[0].GetLiquidType() == LIQUID_VODKA)
            return ingredients[1] && ingredients[1].CanBeDisinfected() && StillHasVodkaForDisinfect(ingredients[0]);

        return super.CanDo(ingredients, player);
    }
}

modded class Bottle_Base
{
    override void SetActions()
    {
        super.SetActions();
        AddAction(ActionDisinfectSelfVodka);
        AddAction(ActionDisinfectTargetVodka);
    }

    // The ItemBase default (250 ml) would empty most of a bottle per use.
    override float GetDisinfectQuantity(int system = 0, Param param1 = null)
    {
        if (GetLiquidType() == LIQUID_VODKA)
            return STILL_VODKA_DISINFECT_ML;

        return super.GetDisinfectQuantity(system, param1);
    }
}
