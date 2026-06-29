# cpp-lob-market-making-simulator

The purpose of this project is to analyze how a variety of market making algorithms perform under different market regimes.

The selected market making algorithms are:
 - Fixed Spread
 - Inventory Skew
 - Volatility Adjusted
 - Inventory and Volatility Adjusted

With the regimes being:
 - Balanced
 - Buy Pressure
 - Sell Pressure
 - High Volatility

## Usage

Build and run the simulator from the project root:

```bash
cmake -S . -B build
cmake --build build
./build/market_sim
```

To generate the leaderboard and plots:

```python
python3 analysis/analyze_results.py
```
