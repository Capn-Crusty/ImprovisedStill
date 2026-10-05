class CfgPatches
{
    class ImprovisedStill_Scripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Scripts", "DZ_Gear_Cooking", "ImprovisedStill_Data"};
    };
};

class CfgMods
{
    class ImprovisedStill
    {
        dir = "ImprovisedStill";
        name = "Improvised Still";
        type = "mod";
        dependencies[] = {"World", "Mission"};

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"ImprovisedStill/Scripts/4_World"};
            };

            class missionScriptModule
            {
                value = "";
                files[] = {"ImprovisedStill/Scripts/5_Mission"};
            };
        };
    };
};
