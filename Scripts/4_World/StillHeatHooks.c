// A still holding only water sleeps instead of checking for heat, so the
// heat sources wake it when they start (a still put on an already burning
// fire or running stove wakes itself when it is moved).
modded class FireplaceBase
{
    override void StartFire(bool force_start = false)
    {
        super.StartFire(force_start);
        ImprovisedStill.WakeStillsIn(this);
    }
}

modded class PortableGasStove
{
    override void OnWorkStart()
    {
        super.OnWorkStart();
        ImprovisedStill.WakeStillsIn(this);
    }
}
