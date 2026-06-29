#include "Simulator.h"

#include <iostream>
#include <vector>

int main()
{
    std::vector<Regime> regimes = {
        Regime::Balanced,
        Regime::BuyPressure,
        Regime::SellPressure,
        Regime::HighVolatility};

    std::vector<StrategyType> strategies = {
        StrategyType::FixedSpread,
        StrategyType::InventorySkew,
        StrategyType::VolatilityAdjusted,
        StrategyType::InventoryAndVolatilityAdjusted};

    int num_events = 10000;
    unsigned int seed = 42;

    for (Regime regime : regimes)
    {
        for (StrategyType strategy : strategies)
        {
            std::cout
                << "Running "
                << strategy_to_string(strategy)
                << " / "
                << regime_to_string(regime)
                << std::endl;

            Simulator simulator(regime, strategy, num_events, seed);
            simulator.run();
            simulator.write_outputs("outputs");

            seed += 7;
        }
    }

    std::cout << "Done. CSV files written to outputs/." << std::endl;

    return 0;
}