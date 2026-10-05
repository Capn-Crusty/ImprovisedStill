modded class PluginRecipesManager
{
    override void RegisterRecipies()
    {
        super.RegisterRecipies();
        RegisterRecipe(new CraftImprovisedStill);
        Print("[ImprovisedStill] Load marker: recipes registered.");
    }
};
