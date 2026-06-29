from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


OUTPUT_DIR = Path("outputs")
PLOT_DIR = OUTPUT_DIR / "plots"
PLOT_DIR.mkdir(parents=True, exist_ok=True)


def load_many(pattern: str) -> pd.DataFrame:
    files = sorted(OUTPUT_DIR.glob(pattern))

    if not files:
        raise FileNotFoundError(
            f"No files found for pattern {pattern}. "
            "Run the C++ simulator first."
        )

    return pd.concat((pd.read_csv(file) for file in files), ignore_index=True)


def max_drawdown(series: pd.Series) -> float:
    peak = series.cummax()
    drawdown = peak - series
    return float(drawdown.max())


def main() -> None:
    states = load_many("states_*.csv")

    rows = []

    for (strategy, regime), group in states.groupby(["strategy", "regime"]):
        final_pnl = float(group["pnl"].iloc[-1])
        avg_abs_inventory = float(group["inventory"].abs().mean())
        max_abs_inventory = int(group["inventory"].abs().max())
        pnl_volatility = float(group["pnl"].diff().std())

        rows.append(
            {
                "strategy": strategy,
                "regime": regime,
                "final_pnl": final_pnl,
                "avg_abs_inventory": avg_abs_inventory,
                "max_abs_inventory": max_abs_inventory,
                "pnl_volatility": pnl_volatility,
                "max_drawdown": max_drawdown(group["pnl"]),
            }
        )

    leaderboard = pd.DataFrame(rows).sort_values(
        ["regime", "final_pnl"],
        ascending=[True, False]
    )

    leaderboard.to_csv(OUTPUT_DIR / "leaderboard.csv", index=False)

    print("\nLeaderboard by regime:")
    print(leaderboard.to_string(index=False))

    winners = leaderboard.loc[
        leaderboard.groupby("regime")["final_pnl"].idxmax()
    ]

    print("\nBest strategy by regime:")
    print(
        winners[
            [
                "regime",
                "strategy",
                "final_pnl",
                "max_drawdown",
                "avg_abs_inventory",
            ]
        ].to_string(index=False)
    )

    for regime, group in states.groupby("regime"):
        plt.figure()

        for strategy, strategy_group in group.groupby("strategy"):
            plt.plot(
                strategy_group["timestamp"],
                strategy_group["pnl"],
                label=strategy
            )

        plt.title(f"PnL by Strategy - {regime}")
        plt.xlabel("Timestamp")
        plt.ylabel("Mark-to-market PnL")
        plt.legend()
        plt.tight_layout()
        plt.savefig(PLOT_DIR / f"pnl_{regime}.png")
        plt.close()

        plt.figure()

        for strategy, strategy_group in group.groupby("strategy"):
            plt.plot(
                strategy_group["timestamp"],
                strategy_group["inventory"],
                label=strategy
            )

        plt.title(f"Inventory by Strategy - {regime}")
        plt.xlabel("Timestamp")
        plt.ylabel("Inventory")
        plt.legend()
        plt.tight_layout()
        plt.savefig(PLOT_DIR / f"inventory_{regime}.png")
        plt.close()

    print(f"\nSaved leaderboard to {OUTPUT_DIR / 'leaderboard.csv'}")
    print(f"Saved plots to {PLOT_DIR}")


if __name__ == "__main__":
    main()