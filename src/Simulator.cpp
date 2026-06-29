#include "Simulator.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>

Simulator::Simulator(
    Regime regime,
    StrategyType strategy_type,
    int num_events,
    unsigned int seed)
    : regime_(regime),
      strategy_type_(strategy_type),
      num_events_(num_events),
      next_order_id_(1),
      fair_price_(100.0),
      last_fair_price_(100.0),
      recent_volatility_(0.0),
      book_(),
      market_maker_(
          strategy_type,
          // parameters to tweak
          0.10,  // base spread
          10,    // quote size
          0.015, // inventory skew
          3.0    // volatility multiplier
          ),
      rng_(seed)
{
    summary_ = {
        strategy_to_string(strategy_type_),
        regime_to_string(regime_),
        0.0,
        0,
        0,
        0,
        0,
        0.0,
        0.0};
}

void Simulator::run()
{
    seed_initial_book();

    double peak_pnl = -std::numeric_limits<double>::infinity();
    double max_drawdown = 0.0;

    for (int t = 1; t <= num_events_; ++t)
    {
        Order background_order = generate_background_order(t);

        auto trades = book_.process_order(background_order);
        process_trades(trades, t);

        apply_price_impact(background_order);

        if (t % 10 == 0)
        {
            auto quotes = market_maker_.generate_quotes(
                fair_price_,
                recent_volatility_,
                t,
                next_order_id_);

            for (const Order &quote : quotes)
            {
                auto quote_trades = book_.process_order(quote);
                process_trades(quote_trades, t);
            }
        }

        log_state(t);

        double pnl = market_maker_.mark_to_market_pnl(fair_price_);

        if (pnl > peak_pnl)
        {
            peak_pnl = pnl;
        }

        max_drawdown = std::max(max_drawdown, peak_pnl - pnl);
        summary_.max_drawdown = max_drawdown;
    }

    compute_summary();
}

void Simulator::seed_initial_book()
{
    for (int i = 1; i <= 20; ++i)
    {
        double bid_price = fair_price_ - 0.05 * i;
        double ask_price = fair_price_ + 0.05 * i;

        book_.process_order(
            Order(
                next_order_id_++,
                Side::Buy,
                OrderType::Limit,
                bid_price,
                50,
                0,
                false));

        book_.process_order(
            Order(
                next_order_id_++,
                Side::Sell,
                OrderType::Limit,
                ask_price,
                50,
                0,
                false));
    }
}

Order Simulator::generate_background_order(int timestamp)
{
    double event_draw = uniform(0.0, 1.0);

    double buy_prob = 0.50;
    double market_prob = 0.60;
    int max_quantity = 20;
    double price_noise = 0.20;

    if (regime_ == Regime::BuyPressure)
    {
        buy_prob = 0.68;
    }

    if (regime_ == Regime::SellPressure)
    {
        buy_prob = 0.32;
    }

    if (regime_ == Regime::HighVolatility)
    {
        market_prob = 0.75;
        max_quantity = 35;
        price_noise = 0.45;
    }

    Side side = uniform(0.0, 1.0) < buy_prob ? Side::Buy : Side::Sell;
    int quantity = random_int(1, max_quantity);

    if (event_draw < market_prob)
    {
        return Order(
            next_order_id_++,
            side,
            OrderType::Market,
            0.0,
            quantity,
            timestamp,
            false);
    }

    double offset = uniform(0.01, price_noise);
    double price = side == Side::Buy
                       ? fair_price_ - offset
                       : fair_price_ + offset;

    return Order(
        next_order_id_++,
        side,
        OrderType::Limit,
        price,
        quantity,
        timestamp,
        false);
}

void Simulator::apply_price_impact(const Order &order)
{
    last_fair_price_ = fair_price_;

    if (order.type == OrderType::Market)
    {
        double impact = regime_ == Regime::HighVolatility ? 0.035 : 0.012;

        if (order.side == Side::Buy)
        {
            fair_price_ += impact;
        }
        else
        {
            fair_price_ -= impact;
        }
    }

    fair_price_ += uniform(-0.005, 0.005);

    if (regime_ == Regime::HighVolatility)
    {
        fair_price_ += uniform(-0.03, 0.03);
    }

    double absolute_price_change = std::abs(fair_price_ - last_fair_price_);

    recent_volatility_ =
        0.94 * recent_volatility_ + 0.06 * absolute_price_change;
}

void Simulator::process_trades(
    const std::vector<Trade> &trades,
    int timestamp)
{
    market_maker_.update_from_trades(trades);

    for (const Trade &trade : trades)
    {
        trade_logs_.push_back(
            {timestamp,
             strategy_to_string(strategy_type_),
             regime_to_string(regime_),
             trade.price,
             trade.quantity,
             trade.market_maker_bought,
             trade.market_maker_sold});

        if (trade.market_maker_bought || trade.market_maker_sold)
        {
            summary_.fills += 1;

            if (trade.market_maker_bought)
            {
                summary_.mm_buy_fills += 1;
            }

            if (trade.market_maker_sold)
            {
                summary_.mm_sell_fills += 1;
            }
        }
    }
}

void Simulator::log_state(int timestamp)
{
    double bid = book_.has_bid()
                     ? book_.best_bid()
                     : std::numeric_limits<double>::quiet_NaN();

    double ask = book_.has_ask()
                     ? book_.best_ask()
                     : std::numeric_limits<double>::quiet_NaN();

    int inventory = market_maker_.inventory();
    double pnl = market_maker_.mark_to_market_pnl(fair_price_);

    summary_.max_abs_inventory = std::max(
        summary_.max_abs_inventory,
        static_cast<double>(std::abs(inventory)));

    state_logs_.push_back(
        {timestamp,
         strategy_to_string(strategy_type_),
         regime_to_string(regime_),
         fair_price_,
         bid,
         ask,
         recent_volatility_,
         inventory,
         market_maker_.cash(),
         pnl,
         book_.total_bid_depth(),
         book_.total_ask_depth()});
}

void Simulator::compute_summary()
{
    summary_.final_pnl = market_maker_.mark_to_market_pnl(fair_price_);
    summary_.final_inventory = market_maker_.inventory();
}

void Simulator::write_outputs(const std::string &output_dir) const
{
    std::filesystem::create_directories(output_dir);

    std::string suffix =
        strategy_to_string(strategy_type_) + "_" + regime_to_string(regime_);

    std::ofstream states(output_dir + "/states_" + suffix + ".csv");

    states
        << "timestamp,strategy,regime,fair_price,best_bid,best_ask,"
        << "recent_volatility,inventory,cash,pnl,bid_depth,ask_depth\n";

    states << std::fixed << std::setprecision(6);

    for (const auto &state : state_logs_)
    {
        states
            << state.timestamp << ","
            << state.strategy << ","
            << state.regime << ","
            << state.fair_price << ","
            << state.best_bid << ","
            << state.best_ask << ","
            << state.recent_volatility << ","
            << state.inventory << ","
            << state.cash << ","
            << state.pnl << ","
            << state.bid_depth << ","
            << state.ask_depth << "\n";
    }

    std::ofstream trades(output_dir + "/trades_" + suffix + ".csv");

    trades
        << "timestamp,strategy,regime,price,quantity,mm_bought,mm_sold\n";

    trades << std::fixed << std::setprecision(6);

    for (const auto &trade : trade_logs_)
    {
        trades
            << trade.timestamp << ","
            << trade.strategy << ","
            << trade.regime << ","
            << trade.price << ","
            << trade.quantity << ","
            << trade.mm_bought << ","
            << trade.mm_sold << "\n";
    }

    std::ofstream summary(output_dir + "/summary_" + suffix + ".csv");

    summary
        << "strategy,regime,final_pnl,final_inventory,fills,"
        << "mm_buy_fills,mm_sell_fills,max_abs_inventory,max_drawdown\n";

    summary << std::fixed << std::setprecision(6);

    summary
        << summary_.strategy << ","
        << summary_.regime << ","
        << summary_.final_pnl << ","
        << summary_.final_inventory << ","
        << summary_.fills << ","
        << summary_.mm_buy_fills << ","
        << summary_.mm_sell_fills << ","
        << summary_.max_abs_inventory << ","
        << summary_.max_drawdown << "\n";
}

double Simulator::uniform(double low, double high)
{
    std::uniform_real_distribution<double> distribution(low, high);
    return distribution(rng_);
}

int Simulator::random_int(int low, int high)
{
    std::uniform_int_distribution<int> distribution(low, high);
    return distribution(rng_);
}