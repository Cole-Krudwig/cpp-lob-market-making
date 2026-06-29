#ifndef ORDER_H
#define ORDER_H

#include <string>

/*
Order types used in the simulator.
*/

enum class Side
{
    Buy,
    Sell
};

enum class OrderType
{
    Limit,
    Market,
    Cancel
};

enum class Regime
{
    Balanced,
    BuyPressure,
    SellPressure,
    HighVolatility
};

enum class StrategyType
{
    FixedSpread,
    InventorySkew,
    VolatilityAdjusted,
    InventoryAndVolatilityAdjusted
};

struct Order
{
    int id;
    Side side;
    OrderType type;
    double price;
    int quantity;
    int timestamp;
    bool is_market_maker_order;

    Order(
        int id_,
        Side side_,
        OrderType type_,
        double price_,
        int quantity_,
        int timestamp_,
        bool is_market_maker_order_ = false)
        : id(id_),
          side(side_),
          type(type_),
          price(price_),
          quantity(quantity_),
          timestamp(timestamp_),
          is_market_maker_order(is_market_maker_order_) {}
};

struct Trade
{
    int timestamp;
    int buy_order_id;
    int sell_order_id;
    double price;
    int quantity;
    bool market_maker_bought;
    bool market_maker_sold;

    Trade(
        int timestamp_,
        int buy_order_id_,
        int sell_order_id_,
        double price_,
        int quantity_,
        bool market_maker_bought_ = false,
        bool market_maker_sold_ = false)
        : timestamp(timestamp_),
          buy_order_id(buy_order_id_),
          sell_order_id(sell_order_id_),
          price(price_),
          quantity(quantity_),
          market_maker_bought(market_maker_bought_),
          market_maker_sold(market_maker_sold_) {}
};

// Helper functions for cleaner CSV output and debugging.

inline std::string side_to_string(Side side)
{
    return side == Side::Buy ? "Buy" : "Sell";
}

inline std::string order_type_to_string(OrderType type)
{
    switch (type)
    {
    case OrderType::Limit:
        return "Limit";
    case OrderType::Market:
        return "Market";
    case OrderType::Cancel:
        return "Cancel";
    default:
        return "Unknown";
    }
}

inline std::string regime_to_string(Regime regime)
{
    switch (regime)
    {
    case Regime::Balanced:
        return "Balanced";
    case Regime::BuyPressure:
        return "BuyPressure";
    case Regime::SellPressure:
        return "SellPressure";
    case Regime::HighVolatility:
        return "HighVolatility";
    default:
        return "Unknown";
    }
}

inline std::string strategy_to_string(StrategyType strategy)
{
    switch (strategy)
    {
    case StrategyType::FixedSpread:
        return "FixedSpread";
    case StrategyType::InventorySkew:
        return "InventorySkew";
    case StrategyType::VolatilityAdjusted:
        return "VolatilityAdjusted";
    case StrategyType::InventoryAndVolatilityAdjusted:
        return "InventoryAndVolatilityAdjusted";
    default:
        return "Unknown";
    }
}

#endif