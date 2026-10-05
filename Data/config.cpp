class CfgPatches
{
    class ImprovisedStill_Data
    {
        units[] = {"ImprovisedStill"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Gear_Cooking",
            "DZ_Gear_Drinks",
            "DZ_Gear_Food",
            "DZ_Gear_Containers",
            "DZ_Gear_Tools",
            "DZ_Gear_Medical"
        };
    };
};

class CfgSlots
{
    class Slot_StillPipe
    {
        name = "StillPipe";
        displayName = "Condenser Pipe";
    };

    class Slot_StillBottle
    {
        name = "StillBottle";
        displayName = "Collection Bottle";
    };

    class Slot_StillThermometer
    {
        name = "StillThermometer";
        displayName = "Thermometer";
    };
};

// Vanilla defines constants for these liquids but no CfgLiquidDefinitions
// entries. Without them a container of either shows ERROR as its contents
// and the server logs NULL m_TemperatureLiquid*Threshold exceptions.
class CfgLiquidDefinitions
{
    class SaltWater
    {
        type = 262144;
        displayName = "Salt Water";
        flammability = -10;
        liquidFreezeThreshold = 0;
        liquidThawThreshold = 0;
        liquidBoilingThreshold = 100;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 0;
            // Dehydration is applied in script (SaltwaterHydrationLossPerMl).
            water = 0;
            nutritionalIndex = 75;
            toxicity = 0;
            digestibility = 2;
            // Saltwater sickness agent (STILL_AGENT_SALTWATER in
            // Scripts/4_World/StillCholera.c); agentsPerDigest 0 = one agent
            // per digested ml.
            agents = 1048576;
            agentsPerDigest = 0;
        };
    };

    class CleanWater
    {
        type = 4194304;
        // Shows as plain water: you cannot tell distilled water from any other
        // (only the still's own tooltip names it, since you watched it come out).
        displayName = "Water";
        flammability = -10;
        liquidFreezeThreshold = 0;
        liquidThawThreshold = 0;
        liquidBoilingThreshold = 100;
        class Nutrition
        {
            fullnessIndex = 1;
            energy = 0;
            water = 100;
            nutritionalIndex = 75;
            toxicity = -0.01;
            digestibility = 2;
        };
    };
};

class CfgVehicles
{
    class Pot;
    class Edible_Base;
    class Inventory_Base;

    // Make vanilla components eligible for the still's dedicated slots.
    class Bottle_Base : Edible_Base
    {
        inventorySlot[] += {"StillBottle"};
    };

    // Vanilla gives pipes and thermometers no temperature; the still heats
    // them, so they need one (no freezing, like glass bottles).
    class Pipe : Inventory_Base
    {
        inventorySlot[] += {"StillPipe"};
        varTemperatureMin = -100;
        varTemperatureMax = 200;
        varTemperatureFreezePoint = -200;
        varTemperatureThawPoint = -200;
    };

    // Optional: improves vodka yield (see ThermometerVodkaBonus).
    class Thermometer : Inventory_Base
    {
        inventorySlot[] += {"StillThermometer"};
        varTemperatureMin = -100;
        varTemperatureMax = 200;
        varTemperatureFreezePoint = -200;
        varTemperatureThawPoint = -200;
    };

    class ImprovisedStill : Pot
    {
        scope = 2;
        displayName = "Improvised Still";
        descriptionShort = "An improvised distillation assembly. Requires a condenser pipe and collection bottle. An attached thermometer improves the spirit yield.";
        // Pot inheritance keeps vanilla cookware slots/behavior.
        attachments[] += {"StillPipe", "StillBottle", "StillThermometer"};
    };
};

