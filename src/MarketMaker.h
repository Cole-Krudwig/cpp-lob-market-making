#ifndef MARKET_MAKER_H
#define MARKET_MAKER_H

#include "Order.h"

#include <vector>

/*
MarketMaker generates bid/ask quotes and tracks its own trading state.
*/

class MarketMaker
{
public:
    MarketMaker(
        StrategyType strategy_type,
        double base_spread,
        int quote_size,
        double inventory_skew,
        double volatility_multiplier);

    std::vector<Order> generate_quotes(
        double mid_price,
        double recent_volatility,
        int timestamp,
        int &next_order_id);

    void update_from_trades(const std::vector<Trade> &trades);

    int inventory() const;
    double cash() const;
    double mark_to_market_pnl(double mid_price) const;

    StrategyType strategy_type() const;

private:
    StrategyType strategy_type_;

    double base_spread_;
    int quote_size_;
    double inventory_skew_;
    double volatility_multiplier_;

    int inventory_;
    double cash_;

    double reservation_price(double mid_price) const;
    double quoted_spread(double recent_volatility) const;
};

#endif
