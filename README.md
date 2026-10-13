# Improvised Still (DayZ mod)

Author: Capn_Crusty

![The Improvised Still on a gas stove, fermenting a potato mash](docs/workshop-preview.png)

Craft a still from a cooking pot and a pipe, then distill any water into clean drinking water, or fruit and potatoes into moonshine, over a fire or a gas stove. Works on vanilla DayZ with no other mods required.

## How to use
1. **Craft:** combine a cooking pot with a pipe, either way round (in hands, in inventory or on the ground). The still lands in your hands with the pipe in its Condenser Pipe slot. It keeps the pot's condition and the pipe's condition.
2. **Bottle:** attach a glass bottle, plastic water bottle, canteen, water pouch or filtering bottle to the Collection Bottle slot. Optionally attach a **thermometer** to the Thermometer slot: while attached it adds 25% to the moonshine yield, and the still's tooltip shows its temperature.
3. **Heat:** put the still on a lit fireplace or fire barrel, or on a lit portable gas stove. It only runs while heated with both the pipe and the bottle attached. A fireplace that has burned out keeps heating the still while its coals are hot; a gas stove stops as soon as it is turned off, and so does a still lifted off the heat.
4. **Load:**
   - **Clean water:** fill the still with any water: sea, pond, river or snow. Any water container (bottles, canteens, pots, cauldrons, jerry cans, the still) can be filled straight from the sea.
   - **Beer:** fill the still with fresh water (salt water will not ferment) and put fruit or potatoes in its cargo, then leave it **unheated** to ferment (15 minutes by default; the tooltip shows "MASH (FERMENTING)"). Each inventory slot of fruit ferments 250 ml of water, so a full 2-litre still needs 8 slots (for example 8 apples or 4 pears); with too little the tooltip shows "MASH (NEEDS MORE FRUIT)". Dried and cooked fruit count like fresh, rotten fruit half, tomatoes, peppers and zucchini a quarter, and a partly eaten item its share; berries and pumpkin count as fruit; burnt or ruined fruit and cannabis do not ferment. When it finishes, the water becomes **beer**: the fruit is used up only as far as the water needs (extra fruit stays, a partly needed item is partly used), and fermenting kills most germs that were in the water. Fermenting is full speed from 20 °C, slower in the cold, and stops at 5 °C or below and above 40 °C (hot mash must cool first); the tooltip shows "MASH (TOO COLD)" or "MASH (TOO HOT)". Heat pauses fermenting, heating early just distills clean water, and taking the fruit out before it finishes starts it over. Fermentation progress survives server restarts.
   - **Moonshine:** heat a still holding beer. It gives 1 ml of moonshine per 3 ml of beer, so a full still nearly fills a 750 ml glass bottle (a full one with a thermometer). Beer from anywhere works, so you can ferment, store the beer, and distill it later.
   - **Heating up:** nothing comes over until the liquid is hot enough: moonshine from 78 °C (alcohol's boiling point), clean water at 100 °C. A thermometer attachment shows the temperature.
5. **Output:** every 30 seconds up to 50 ml moves into the bottle. The still's tooltip shows the collection bottle's contents and fill level (and a missing pipe or bottle), which helps when it sits on a fireplace where its slots are not shown. The still pauses if the bottle is full or already holds a different liquid. Plastic water bottles take a little damage with each batch.

## Liquids
- **Salt Water** shows its name in the item tooltip (you would taste it at once).
- **Distilled water** is ordinary vanilla Water with no disease, so it is more reliable than boiling. It mixes with any other water, works as car radiator coolant and puts out fires (salt water, mash and beer are not coolant). Poured into a vessel that already holds water, it keeps whatever that water carried: clean into dirty is dirty. Clean Water made by versions before 1.0.3 still works the same.
- **Salt Water** dehydrates you (each ml costs about 1.5 ml of hydration), makes you vomit once about 500 ml is in your stomach, and causes **saltwater sickness** after about 300 ml: cholera-like vomiting and water loss, worse the more you drink. Antibiotics do not cure it; a completed saline IV (the whole bag, on yourself or another player) does. Real cholera is unchanged and still needs tetracycline.

## Beer
Fermented mash is vanilla beer wherever it goes: pour it, carry it, drink it (hydration and energy) or store it, and it never spoils. Each batch has a chance (20% by default, 60% with any rotten fruit) to go bad as it becomes beer; drinking a bad batch can give food poisoning, and the germs travel with the beer when it is poured. Mash still fermenting in a still is water until it finishes, so pouring it out gives water.

## Drinking
Moonshine and beer make you drunk as your stomach absorbs them; food in the stomach slows it down. The level wears off over time and is saved with your character, so logging out does not sober you up.

| Level | About (empty stomach) | Effects |
|---|---|---|
| Tipsy | 50 ml of moonshine | Mild pulsing blur, a warm feeling (heat buffer rises), extra water loss |
| Drunk | 150 ml | Strong blur, the occasional stumble, pain relief (vanilla painkiller effect) |
| Very drunk | 300 ml | Heavier blur and stumbles, a chance to vomit |
| Blackout | 500 ml | You pass out until shock recovers |

**Blackout warning:** passing out is vanilla unconsciousness. If you disconnect, quit or your game crashes while blacked out, vanilla kills your character (its anti-combat-log rule). A clean server restart is safe.

## Cooking
The still cooks food like a normal pot. While it holds liquid, boiled food stays boiled and does not burn; once it runs dry, food can bake and burn as in any pot.

## Moonshine as disinfectant
Moonshine in a drink container works like alcohol tincture, 25 ml per use:
- Disinfect yourself or another player.
- Disinfect items (rags, clothing, tools) by combining them with the bottle.
- Wash bloody hands.


## Server setup
Copy the key from `keys` to your server's keys folder, and add `improvisedstill_types.xml` (in the mod folder) to your mission: put it in a CE folder registered in `cfgeconomycore.xml` (e.g. `<ce folder="customtypes"><file name="improvisedstill_types.xml" type="types" /></ce>`), or copy its entry into `db/types.xml`. The still is crafted only (never spawns as loot); the entry sets how long an untouched still lasts before cleanup (7 days by default).

## Server settings
On first start the server creates `<server profile>/ImprovisedStill/config.json` with the defaults below. Edit it and restart the server to apply changes; missing keys keep their defaults.

| Setting | Default | Meaning |
|---|---|---|
| `SpiritName` | Moonshine | What players see the spirit called (it is vanilla's vodka liquid underneath; the settings below keep "Vodka" in their names) |
| `SaltwaterSicknessStartAgents` | 100 | Agents at which saltwater sickness starts |
| `SaltwaterSicknessEndAgents` | 20 | Agents at which it ends |
| `SaltwaterAgentsPerMl` | 0.35 | Agents taken in per ml of salt water digested |
| `SaltwaterAgentGrowth` | 0.3 | How fast agents multiply (vanilla cholera 0.15) |
| `SaltwaterAgentDieOff` | 0.2 | Agents lost per second when the immune system wins (vanilla cholera 0.45) |
| `SaltwaterVomitStomachMl` | 500 | Salt water in the stomach that causes vomiting; 0 disables |
| `SaltwaterHydrationLossPerMl` | 1.5 | Hydration lost per ml of salt water digested |
| `SalineResistanceSeconds` | 0 | Immunity to saltwater sickness after a completed saline IV; 0 disables |
| `FermentationMinutes` | 15 | Unheated time water + fruit needs before the water becomes beer; 0 = instant |
| `WaterPerFruitSlot` | 250 | ml of water each inventory slot of fruit or potato can ferment; not enough fruit = no fermenting |
| `RottenFruitVodkaYield` | 0.5 | How much a rotten fruit or potato counts toward `WaterPerFruitSlot` |
| `LowSugarVegetableVodkaYield` | 0.25 | How much tomatoes, peppers and zucchini count toward `WaterPerFruitSlot` |
| `FermentKillsGermsChance` | 0.75 | Chance (0-1) that fermenting kills each germ already in the water |
| `MashFoodPoisonChance` | 0.2 | Chance (0-1) a batch goes bad as it becomes beer |
| `RottenMashFoodPoisonChance` | 0.6 | The same chance when any of the fruit or potatoes are rotten (burnt ones do not ferment) |
| `FermentMinTemperature` | 5 | Liquid temperature (°C) at or below which fermentation stops |
| `FermentFullSpeedTemperature` | 20 | Full fermentation speed from this temperature; slower below it |
| `FermentMaxTemperature` | 40 | Above this (hot mash after heating) fermentation stops until it cools |
| `BeerPerSpiritMl` | 3 | ml of beer used up per ml of moonshine distilled |
| `MashDistillTemperature` | 78 | Liquid temperature (°C) at which beer starts giving moonshine; water distills at 100 °C |
| `FireplaceResidualHeatTemperature` | 100 | A fireplace that has burned out keeps heating the still while it is at least this hot (°C) |
| `ThermometerVodkaBonus` | 0.25 | Extra moonshine while a thermometer is attached (0.25 = +25%) |
| `VodkaAlcoholPerMl` | 1.0 | Intoxication units per ml of moonshine digested |
| `BeerAlcoholPerMl` | 0.1 | Units per ml of beer digested |
| `TipsyUnits` / `DrunkUnits` / `VeryDrunkUnits` | 50 / 150 / 300 | Thresholds for each level |
| `BlackoutUnits` | 500 | Passing out; 0 disables |
| `SoberingUnitsPerMinute` | 15 | How fast it wears off |
| `AlcoholWaterLossPerUnit` | 0.01 | Extra water lost per second, per unit |
| `VeryDrunkVomitChance` | 0.075 | Vomit chance per 3 s while very drunk |
| `PainReliefFromTier` | 2 | Painkiller effect from this level up (1 tipsy, 2 drunk, 3 very drunk); 0 disables |
| `AlcoholWarmthPerSecond` | 0.02 | Heat buffer gained per second while tipsy or more; 0 disables |

## Planned
- A dedicated still model built from vanilla parts (cooking pot body, the attached pipe and bottle shown in place).

## Layout
- `Data/` item, slot and liquid config (Data.pbo, prefix `ImprovisedStill/Data`)
- `Scripts/` Enforce Script (Scripts.pbo, prefix `ImprovisedStill\Scripts`): `4_World` gameplay, `5_Mission` tooltip names

## Releases
Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3813055823

## License
See [LICENSE](LICENSE). Requires DayZ; not affiliated with or endorsed by Bohemia Interactive.
