#include "MarketMaker.h"

#include <algorithm>

MarketMaker::MarketMaker(
    StrategyType strategy_type,
    double base_spread,
    int quote_size,
    double inventory_skew,
    double volatility_multiplier)
    : strategy_type_(strategy_type),
      base_spread_(base_spread),
      quote_size_(quote_size),
      inventory_skew_(inventory_skew),
      volatility_multiplier_(volatility_multiplier),
      inventory_(0),
      cash_(0.0) {}

std::vector<Order> MarketMaker::generate_quotes(
    double mid_price,
    double recent_volatility,
    int timestamp,
    int &next_order_id)
{
    double center = reservation_price(mid_price);
    double spread = quoted_spread(recent_volatility);

    double bid = center - spread / 2.0;
    double ask = center + spread / 2.0;

    std::vector<Order> quotes;

    quotes.emplace_back(
        next_order_id++,
        Side::Buy,
        OrderType::Limit,
        bid,
        quote_size_,
        timestamp,
        true);

    quotes.emplace_back(
        next_order_id++,
        Side::Sell,
        OrderType::Limit,
        ask,
        quote_size_,
        timestamp,
        true);

    return quotes;
}

void MarketMaker::update_from_trades(const std::vector<Trade> &trades)
{
    for (const Trade &trade : trades)
    {
        if (trade.market_maker_bought)
        {
            inventory_ += trade.quantity;
            cash_ -= trade.price * trade.quantity;
        }

        if (trade.market_maker_sold)
        {
            inventory_ -= trade.quantity;
            cash_ += trade.price * trade.quantity;
        }
    }
}

int MarketMaker::inventory() const
{
    return inventory_;
}

double MarketMaker::cash() const
{
    return cash_;
}

double MarketMaker::mark_to_market_pnl(double mid_price) const
{
    return cash_ + inventory_ * mid_price;
}

StrategyType MarketMaker::strategy_type() const
{
    return strategy_type_;
}

double MarketMaker::reservation_price(double mid_price) const
{
    if (
        strategy_type_ == StrategyType::InventorySkew ||
        strategy_type_ == StrategyType::InventoryAndVolatilityAdjusted)
    {
        return mid_price - inventory_skew_ * static_cast<double>(inventory_);
    }

    return mid_price;
}

double MarketMaker::quoted_spread(double recent_volatility) const
{
    if (
        strategy_type_ == StrategyType::VolatilityAdjusted ||
        strategy_type_ == StrategyType::InventoryAndVolatilityAdjusted)
    {
        return std::max(
            base_spread_,
            base_spread_ + volatility_multiplier_ * recent_volatility);
    }

    return base_spread_;
}