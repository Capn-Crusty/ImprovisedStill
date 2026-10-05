// Vanilla food stage rules turn any cooked food (boiled, baked, dried) into
// Burned with further boiling. In a still that still holds liquid, raw food
// boils and everything else is left as it is; once the still runs dry it
// cooks (and can burn) like in any pot. Also stops vanilla's steam loss from
// a working still.
modded class Cooking
{
    override void ProcessItemToCook(notnull ItemBase pItem, ItemBase cookingEquip, Param2<CookingMethodType, float> pCookingMethod, out Param2<bool, bool> pStateFlags)
    {
        // Vanilla boils liquid off into the air. A still with its pipe and a
        // bottle with room sends the steam down the pipe instead (distilling
        // accounts for it), so nothing is lost.
        ImprovisedStill still = ImprovisedStill.Cast(pItem);
        if (still && pItem == cookingEquip && still.HasCondenserPipe() && still.HasCollectionBottle())
        {
            ItemBase bottle = still.GetCollectionBottle();
            if (bottle.GetQuantity() < bottle.GetQuantityMax())
            {
                float quantity = still.GetQuantity();
                super.ProcessItemToCook(pItem, cookingEquip, pCookingMethod, pStateFlags);
                if (still.GetQuantity() < quantity)
                    still.SetQuantity(quantity);
                return;
            }
        }

        if (ImprovisedStill.Cast(cookingEquip) && pItem != cookingEquip && cookingEquip.GetQuantity() > 0)
        {
            Edible_Base food = Edible_Base.Cast(pItem);
            if (food && food.GetFoodStageType() != FoodStageType.RAW)
            {
                // Still warms up with the liquid, without cooking further.
                AddTemperatureToItem(pItem, cookingEquip, 0);
                return;
            }
        }

        super.ProcessItemToCook(pItem, cookingEquip, pCookingMethod, pStateFlags);
    }
}
