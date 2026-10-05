// Server settings, read from <server profile>/ImprovisedStill/config.json.
// The file is created with these defaults on first start; missing keys keep
// their defaults. Only the server reads it.
class StillSettings
{
    int Version = 1;

    string SpiritName = "Moonshine";            // what players see the still's spirit called (vanilla's vodka liquid); max 32 characters

    // Saltwater sickness
    float SaltwaterSicknessStartAgents = 100;   // agents at which the sickness starts
    float SaltwaterSicknessEndAgents = 20;      // agents at which it ends
    float SaltwaterAgentsPerMl = 0.35;          // agents taken in per ml of salt water digested
    float SaltwaterAgentGrowth = 0.3;           // how fast agents multiply (vanilla cholera 0.15)
    float SaltwaterAgentDieOff = 0.2;           // agents lost per second when the immune system wins (vanilla cholera 0.45)
    float SaltwaterVomitStomachMl = 500;        // salt water in the stomach that causes vomiting; 0 disables
    float SaltwaterHydrationLossPerMl = 1.5;    // hydration lost per ml of salt water digested
    float SalineResistanceSeconds = 0;          // after a completed saline IV, immunity to saltwater sickness; 0 disables

    // Fermentation
    float FermentationMinutes = 15;             // unheated time water + fruit/potatoes must sit before they distill to vodka; 0 = instant
    float MashFoodPoisonChance = 0.2;           // chance (0-1) a batch goes bad when it becomes ready; drinking a bad batch can cause food poisoning
    float SpiritPerCargoSlot = 100;             // ml of spirit per inventory slot the fruit/potato takes up
    float MashWaterPerSpiritMl = 1;             // ml of mash water used up per ml of spirit (realistically 5-10)
    float RottenMashFoodPoisonChance = 0.6;     // the same chance when the fruit/potatoes are rotten (burnt ones do not ferment at all)
    float RottenFruitVodkaYield = 0.5;          // vodka from rotten fruit/potatoes compared with fresh, dried or cooked (0.5 = half)
    float LowSugarVegetableVodkaYield = 0.25;   // vodka from tomatoes, peppers and zucchini compared with fruit (cannabis does not ferment)
    float TopUpFermentationFraction = 0.5;      // fresh fruit/potatoes added to ready mash send it back to fermenting for this share of FermentationMinutes; 0 = stays ready
    float FermentMinTemperature = 5;            // liquid temperature (°C) at or below which fermentation (and spoiling) stops
    float FermentFullSpeedTemperature = 20;     // full speed from here; slower between the minimum and this
    float FermentMaxTemperature = 40;           // above this (hot mash after heating) fermentation stops until it cools
    float MashSpoilMinutes = 120;               // unheated time ready mash keeps before it spoils (fruit rots, no vodka, can give food poisoning); 0 = never
    float MashDistillTemperature = 78;          // liquid temperature (°C) at which ready mash starts giving vodka; water distills at its boiling point

    // Thermometer attachment
    float ThermometerVodkaBonus = 0.25;         // extra vodka per fruit/potato with a thermometer attached (0.25 = +25%)

    // Drinking alcohol (intoxication, in "units": 1 ml of vodka = 1 unit)
    float VodkaAlcoholPerMl = 1.0;              // units per ml of vodka digested
    float BeerAlcoholPerMl = 0.1;               // units per ml of beer (and ready mash) digested
    float TipsyUnits = 50;
    float DrunkUnits = 150;
    float VeryDrunkUnits = 300;
    float BlackoutUnits = 500;                  // passes out (vanilla shock unconsciousness); 0 disables
    float SoberingUnitsPerMinute = 15;
    float AlcoholWaterLossPerUnit = 0.01;       // extra water lost per second, per unit
    float VeryDrunkVomitChance = 0.075;         // per 3 s while very drunk
    float PainReliefFromTier = 2;               // painkiller effect from this tier up (1 tipsy, 2 drunk, 3 very drunk); 0 disables
    float AlcoholWarmthPerSecond = 0.02;        // heat buffer gained per second while tipsy or more; 0 disables

    protected static ref StillSettings s_Instance;

    static StillSettings Get()
    {
        if (!s_Instance)
            s_Instance = Load();
        return s_Instance;
    }

    protected static StillSettings Load()
    {
        StillSettings settings = new StillSettings();
        if (!g_Game || !g_Game.IsServer())
            return settings;

        string dir = "$profile:ImprovisedStill";
        string path = dir + "/config.json";
        string error;

        if (FileExist(path))
        {
            if (!JsonFileLoader<StillSettings>.LoadFile(path, settings, error))
            {
                Print("[ImprovisedStill] Could not read " + path + ", using defaults: " + error);
                return new StillSettings();
            }
            Print("[ImprovisedStill] Settings loaded from " + path);
        }
        else
        {
            if (!FileExist(dir))
                MakeDirectory(dir);
            if (JsonFileLoader<StillSettings>.SaveFile(path, settings, error))
                Print("[ImprovisedStill] Created " + path + " with defaults");
            else
                Print("[ImprovisedStill] Could not create " + path + ": " + error);
        }

        // Save back so keys added in newer versions appear in the file.
        JsonFileLoader<StillSettings>.SaveFile(path, settings, error);
        return settings;
    }
}
