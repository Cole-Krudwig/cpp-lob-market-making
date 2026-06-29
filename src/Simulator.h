#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "MarketMaker.h"
#include "OrderBook.h"

#include <random>
#include <string>
#include <vector>

struct StateLog
{
    int timestamp;
    std::string strategy;
    std::string regime;
    double fair_price;
    double best_bid;
    double best_ask;
    double recent_volatility;
    int inventory;
    double cash;
    double pnl;
    int bid_depth;
    int ask_depth;
};

struct TradeLog
{
    int timestamp;
    std::string strategy;
    std::string regime;
    double price;
    int quantity;
    bool mm_bought;
    bool mm_sold;
};

struct SummaryLog
{
    std::string strategy;
    std::string regime;
    double final_pnl;
    int final_inventory;
    int fills;
    int mm_buy_fills;
    int mm_sell_fills;
    double max_abs_inventory;
    double max_drawdown;
};

class Simulator
{
public:
    Simulator(
        Regime regime,
        StrategyType strategy_type,
        int num_events,
        unsigned int seed);

    void run();
    void write_outputs(const std::string &output_dir) const;

private:
    Regime regime_;
    StrategyType strategy_type_;
    int num_events_;
    int next_order_id_;

    double fair_price_;
    double last_fair_price_;
    double recent_volatility_;

    OrderBook book_;
    MarketMaker market_maker_;
    std::mt19937 rng_;

    std::vector<StateLog> state_logs_;
    std::vector<TradeLog> trade_logs_;
    SummaryLog summary_;

    Order generate_background_order(int timestamp);
    void seed_initial_book();
    void apply_price_impact(const Order &order);
    void process_trades(const std::vector<Trade> &trades, int timestamp);
    void log_state(int timestamp);
    void compute_summary();

    double uniform(double low, double high);
    int random_int(int low, int high);
};

#endif