// Drinking from a still holding ready mash goes into the stomach as beer.
modded class PlayerBase
{
    override bool Consume(PlayerConsumeData data)
    {
        ImprovisedStill still = ImprovisedStill.Cast(data.m_Source);
        if (!still)
            return super.Consume(data);

        still.SetDrinkingMash(true);
        bool consumed = super.Consume(data);
        still.SetDrinkingMash(false);
        return consumed;
    }
}
