class CraftImprovisedStill : RecipeBase
{
    protected int m_PotIngredientIndex;
    protected int m_PipeIngredientIndex;

    override void Init()
    {
        m_Name = "Assemble improvised still";
        m_IsInstaRecipe = false;
        m_AnimationLength = 2;
        m_Specialty = 0.02;

        InsertIngredient(0, "Pot");
        InsertIngredient(1, "Pipe");

        // RecipeBase leaves these at 0, which means "pristine only" (damage)
        // and "empty only" (quantity). Allow any condition short of ruined,
        // like vanilla recipes; the pot must still be empty.
        m_MinDamageIngredient[0] = -1;
        m_MaxDamageIngredient[0] = 3;
        m_MinQuantityIngredient[0] = -1;
        m_MaxQuantityIngredient[0] = 0;
        m_MinDamageIngredient[1] = -1;
        m_MaxDamageIngredient[1] = 3;
        m_MinQuantityIngredient[1] = -1;
        m_MaxQuantityIngredient[1] = -1;

        AddResult("ImprovisedStill");
        m_ResultSetHealth[0] = -1;
        m_ResultInheritsColor[0] = -1;
        m_ResultSetQuantity[0] = -1;
        // CanDo finds the pot and the pipe among the ingredients and sets these.
        m_ResultReplacesIngredient[0] = -1;
        m_ResultInheritsHealth[0] = -1;
        // Spawn on the ground: an item stored in cargo cannot take
        // attachments, so the pipe could not be fitted to a still in a backpack.
        m_ResultToInventory[0] = -2;
        m_IngredientDestroy[0] = false;
        m_IngredientDestroy[1] = false;
    }

    override bool CanDo(ItemBase ingredients[], PlayerBase player)
    {
        if (!ingredients[0] || !ingredients[1])
            return false;

        m_PotIngredientIndex = -1;
        m_PipeIngredientIndex = -1;
        m_IngredientDestroy[0] = false;
        m_IngredientDestroy[1] = false;

        for (int i = 0; i < 2; i++)
        {
            if (ingredients[i].IsKindOf("Pot") && !ingredients[i].IsKindOf("ImprovisedStill"))
                m_PotIngredientIndex = i;
            else if (ingredients[i].IsKindOf("Pipe"))
                m_PipeIngredientIndex = i;
        }

        if (m_PotIngredientIndex < 0 || m_PipeIngredientIndex < 0)
            return false;

        // Replace the actual pot regardless of whether it was held or targeted.
        m_ResultReplacesIngredient[0] = m_PotIngredientIndex;
        m_ResultInheritsHealth[0] = m_PotIngredientIndex;
        m_IngredientDestroy[m_PotIngredientIndex] = true;
        m_IngredientDestroy[m_PipeIngredientIndex] = false;
        m_IngredientSetHealth[m_PipeIngredientIndex] = -1;

        return super.CanDo(ingredients, player);
    }

    override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
    {
        if (m_PotIngredientIndex < 0 || m_PipeIngredientIndex < 0)
            return;

        if (!results[0] || !ingredients[m_PipeIngredientIndex])
            return;

        ItemBase still = results[0];
        ItemBase pipe = ingredients[m_PipeIngredientIndex];

        // Inventory moves fail while the craft action still holds the
        // ingredients, so finish assembly just after the recipe completes.
        g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(FinishAssembly, 250, false, still, pipe, player);
    }

    // Attach the existing pipe (keeping its condition) from hands, inventory
    // or the ground, then put the still in the player's empty hands. If the
    // pipe cannot be attached it is left where it is.
    protected void FinishAssembly(ItemBase still, ItemBase pipe, PlayerBase player)
    {
        if (!still)
            return;

        AttachPipe(still, pipe, player, 10);
        TakeStillToHands(still, player, 10);
    }

    // Retries for up to about 2.5 s, since moves can be refused while the
    // craft action is still finishing.
    protected void AttachPipe(ItemBase still, ItemBase pipe, PlayerBase player, int triesLeft)
    {
        if (!still || !pipe || pipe.GetHierarchyParent() == still)
            return;

        InventoryLocation pipeLoc = new InventoryLocation;
        pipe.GetInventory().GetCurrentInventoryLocation(pipeLoc);

        // A pipe held in hands must be released through the player's hand
        // state machine; from the ground or inventory the still takes it
        // directly.
        bool attached;
        if (player && pipeLoc.GetType() == InventoryLocationType.HANDS)
            attached = player.ServerTakeEntityToTargetAttachment(still, pipe);
        else
            attached = still.ServerTakeEntityAsAttachment(pipe);

        if (attached)
            return;

        if (triesLeft > 0)
        {
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(AttachPipe, 250, false, still, pipe, player, triesLeft - 1);
            return;
        }

        Print("[ImprovisedStill] Pipe attachment failed; pipe was left in place. pipe at " + typename.EnumToString(InventoryLocationType, pipeLoc.GetType()));
    }

    // A pipe attached from the hands leaves them a moment later, so wait for
    // empty hands (up to about 2.5 s) before handing the still to the player.
    protected void TakeStillToHands(ItemBase still, PlayerBase player, int triesLeft)
    {
        if (!still || !player || !player.IsAlive() || still.GetHierarchyParent())
            return;

        if (!player.GetItemInHands())
        {
            player.ServerTakeEntityToHands(still);
            return;
        }

        if (triesLeft > 0)
            g_Game.GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(TakeStillToHands, 250, false, still, player, triesLeft - 1);
    }
};
