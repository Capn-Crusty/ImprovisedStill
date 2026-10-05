// Vanilla food stage rules turn Boiled food into Burned with further boiling.
// In a still that still holds liquid, food stops at Boiled; once the still
// runs dry it cooks (and can burn) like in any pot.
modded class Cooking
{
    override void ProcessItemToCook(notnull ItemBase pItem, ItemBase cookingEquip, Param2<CookingMethodType, float> pCookingMethod, out Param2<bool, bool> pStateFlags)
    {
        if (ImprovisedStill.Cast(cookingEquip) && pItem != cookingEquip && cookingEquip.GetQuantity() > 0)
        {
            Edible_Base food = Edible_Base.Cast(pItem);
            if (food && food.GetFoodStageType() == FoodStageType.BOILED)
                return;
        }

        super.ProcessItemToCook(pItem, cookingEquip, pCookingMethod, pStateFlags);
    }
}
