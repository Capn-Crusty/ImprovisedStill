// Vanilla names liquids in the item tooltip from a hardcoded switch and
// shows "ERROR" for any type it does not list, including salt and clean water.
modded class InspectMenuNew
{
    override static void UpdateItemInfoLiquidType(Widget root_widget, EntityAI item)
    {
        super.UpdateItemInfoLiquidType(root_widget, item);

        ItemBase itemBase = ItemBase.Cast(item);
        if (!itemBase || itemBase.GetQuantity() <= 0 || !itemBase.IsLiquidContainer())
            return;

        // Vanilla writes this line in capitals.
        ImprovisedStill still = ImprovisedStill.Cast(itemBase);
        if (still && still.GetFermentState() == ImprovisedStill.STILL_FERMENT_FERMENTING)
        {
            string fermenting = "MASH (FERMENTING)";
            if (still.GetFermentPause() == ImprovisedStill.STILL_PAUSE_COLD)
                fermenting = "MASH (TOO COLD)";
            else if (still.GetFermentPause() == ImprovisedStill.STILL_PAUSE_HOT)
                fermenting = "MASH (TOO HOT)";
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", fermenting, Colors.COLOR_LIQUID);
            return;
        }
        if (still && still.GetFermentState() == ImprovisedStill.STILL_FERMENT_READY)
        {
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", "MASH (READY)", Colors.COLOR_LIQUID);
            return;
        }
        if (still && still.GetFermentState() == ImprovisedStill.STILL_FERMENT_SPOILED)
        {
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", "MASH (SPOILED)", Colors.COLOR_LIQUID);
            return;
        }

        int liquidType = itemBase.GetLiquidType();
        if (liquidType == LIQUID_SALTWATER)
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", "SALT WATER", Colors.COLOR_LIQUID);
        else if (liquidType == LIQUID_CLEANWATER)
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", "#inv_inspect_water", Colors.COLOR_LIQUID); // exactly vanilla's water
        else if (liquidType == LIQUID_VODKA)
        {
            string spirit = StillSpiritName.Get();
            spirit.ToUpper();
            WidgetTrySetText(root_widget, "ItemLiquidTypeWidget", spirit, Colors.COLOR_LIQUID);
        }
    }

    // On a fireplace the inventory shows the still only as an icon, without
    // its pipe and bottle slots, so its description reports them instead.
    override static void UpdateItemInfo(Widget root_widget, EntityAI item)
    {
        super.UpdateItemInfo(root_widget, item);

        ImprovisedStill still = ImprovisedStill.Cast(item);
        if (!still || !root_widget)
            return;

        string status;
        if (!still.GetCondenserPipe())
            status += "\nNo condenser pipe.";

        ItemBase bottle = still.GetCollectionBottle();
        if (!bottle)
            status += "\nNo collection bottle.";
        else if (bottle.GetQuantity() <= 0)
            status += "\nCollection bottle: empty.";
        else
            status += "\nCollection bottle: " + StillLiquidName(bottle.GetLiquidType()) + ", " + Math.Round(bottle.GetQuantity() / bottle.GetQuantityMax() * 100) + "%.";

        // Vanilla lets a pot on the fire run past 100 °C; liquid in it cannot.
        if (still.GetThermometer())
        {
            float temperature = still.GetTemperature();
            if (still.GetQuantity() > 0)
                temperature = Math.Min(temperature, 100.0);
            status += "\nTemperature: " + Math.Round(temperature) + " °C.";
        }

        WidgetTrySetText(root_widget, "ItemDescWidget", still.GetTooltip() + "\n" + status);
    }

    // The liquid's own display name from cfgLiquidDefinitions, translated.
    static string StillLiquidName(int liquidType)
    {
        if (liquidType == LIQUID_VODKA)
            return StillSpiritName.Get();
        if (liquidType == LIQUID_CLEANWATER)
            return "Clean Water"; // only the still names it: you watched it come out

        if (!Liquid.m_LiquidInfosByType.Contains(liquidType))
            return "unknown liquid";

        // Some vanilla names are marked untranslated ("$UNT$Vodka").
        string name = Widget.TranslateString(Liquid.GetDisplayName(liquidType));
        name.Replace("$UNT$", "");
        return name;
    }
};
