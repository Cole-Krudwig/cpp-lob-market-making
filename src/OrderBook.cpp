#include "OrderBook.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

OrderBook::OrderBook() = default;

std::vector<Trade> OrderBook::process_order(const Order &order)
{
    if (order.type == OrderType::Limit)
    {
        return process_limit_order(order);
    }

    if (order.type == OrderType::Market)
    {
        return process_market_order(order);
    }

    if (order.type == OrderType::Cancel)
    {
        cancel_order(order.id);
        return {};
    }

    return {};
}

std::vector<Trade> OrderBook::process_limit_order(Order order)
{
    std::vector<Trade> trades;

    if (order.quantity <= 0)
    {
        return trades;
    }

    if (order.side == Side::Buy)
    {
        // A buy limit order crosses if its price is >= best ask.
        while (order.quantity > 0 && has_ask() && order.price >= best_ask())
        {
            std::vector<Trade> new_trades = match_buy_order(order);
            trades.insert(trades.end(), new_trades.begin(), new_trades.end());
        }
    }
    else
    {
        // A sell limit order crosses if its price is <= best bid.
        while (order.quantity > 0 && has_bid() && order.price <= best_bid())
        {
            std::vector<Trade> new_trades = match_sell_order(order);
            trades.insert(trades.end(), new_trades.begin(), new_trades.end());
        }
    }

    // If the limit order is not fully filled, the remainder rests in the book.
    if (order.quantity > 0)
    {
        add_resting_order(order);
    }

    remove_empty_price_levels();
    return trades;
}

std::vector<Trade> OrderBook::process_market_order(Order order)
{
    std::vector<Trade> trades;

    if (order.quantity <= 0)
    {
        return trades;
    }

    if (order.side == Side::Buy)
    {
        while (order.quantity > 0 && has_ask())
        {
            std::vector<Trade> new_trades = match_buy_order(order);
            trades.insert(trades.end(), new_trades.begin(), new_trades.end());
        }
    }
    else
    {
        while (order.quantity > 0 && has_bid())
        {
            std::vector<Trade> new_trades = match_sell_order(order);
            trades.insert(trades.end(), new_trades.begin(), new_trades.end());
        }
    }

    remove_empty_price_levels();
    return trades;
}

std::vector<Trade> OrderBook::match_buy_order(Order &incoming_order)
{
    std::vector<Trade> trades;

    if (!has_ask() || incoming_order.quantity <= 0)
    {
        return trades;
    }

    auto best_ask_it = asks_.begin();
    auto &resting_orders = best_ask_it->second;

    while (incoming_order.quantity > 0 && !resting_orders.empty())
    {
        Order &resting_sell = resting_orders.front();

        int trade_quantity = std::min(incoming_order.quantity, resting_sell.quantity);
        double trade_price = resting_sell.price;

        bool market_maker_bought =
            incoming_order.is_market_maker_order && incoming_order.side == Side::Buy;

        bool market_maker_sold =
            resting_sell.is_market_maker_order && resting_sell.side == Side::Sell;

        trades.emplace_back(
            incoming_order.timestamp,
            incoming_order.id,
            resting_sell.id,
            trade_price,
            trade_quantity,
            market_maker_bought,
            market_maker_sold);

        incoming_order.quantity -= trade_quantity;
        resting_sell.quantity -= trade_quantity;

        if (resting_sell.quantity == 0)
        {
            order_location_.erase(resting_sell.id);
            resting_orders.pop_front();
        }
    }

    if (resting_orders.empty())
    {
        asks_.erase(best_ask_it);
    }

    return trades;
}

std::vector<Trade> OrderBook::match_sell_order(Order &incoming_order)
{
    std::vector<Trade> trades;

    if (!has_bid() || incoming_order.quantity <= 0)
    {
        return trades;
    }

    auto best_bid_it = bids_.begin();
    auto &resting_orders = best_bid_it->second;

    while (incoming_order.quantity > 0 && !resting_orders.empty())
    {
        Order &resting_buy = resting_orders.front();

        int trade_quantity = std::min(incoming_order.quantity, resting_buy.quantity);
        double trade_price = resting_buy.price;

        bool market_maker_bought =
            resting_buy.is_market_maker_order && resting_buy.side == Side::Buy;

        bool market_maker_sold =
            incoming_order.is_market_maker_order && incoming_order.side == Side::Sell;

        trades.emplace_back(
            incoming_order.timestamp,
            resting_buy.id,
            incoming_order.id,
            trade_price,
            trade_quantity,
            market_maker_bought,
            market_maker_sold);

        incoming_order.quantity -= trade_quantity;
        resting_buy.quantity -= trade_quantity;

        if (resting_buy.quantity == 0)
        {
            order_location_.erase(resting_buy.id);
            resting_orders.pop_front();
        }
    }

    if (resting_orders.empty())
    {
        bids_.erase(best_bid_it);
    }

    return trades;
}

void OrderBook::add_resting_order(const Order &order)
{
    if (order.quantity <= 0)
    {
        return;
    }

    if (order.side == Side::Buy)
    {
        bids_[order.price].push_back(order);
    }
    else
    {
        asks_[order.price].push_back(order);
    }

    order_location_[order.id] = {order.side, order.price};
}

bool OrderBook::cancel_order(int order_id)
{
    auto location_it = order_location_.find(order_id);

    if (location_it == order_location_.end())
    {
        return false;
    }

    Side side = location_it->second.first;
    double price = location_it->second.second;

    if (side == Side::Buy)
    {
        auto price_level_it = bids_.find(price);

        if (price_level_it == bids_.end())
        {
            order_location_.erase(order_id);
            return false;
        }

        auto &orders = price_level_it->second;
        auto order_it = std::find_if(
            orders.begin(),
            orders.end(),
            [order_id](const Order &order)
            {
                return order.id == order_id;
            });

        if (order_it != orders.end())
        {
            orders.erase(order_it);
        }

        if (orders.empty())
        {
            bids_.erase(price_level_it);
        }
    }
    else
    {
        auto price_level_it = asks_.find(price);

        if (price_level_it == asks_.end())
        {
            order_location_.erase(order_id);
            return false;
        }

        auto &orders = price_level_it->second;
        auto order_it = std::find_if(
            orders.begin(),
            orders.end(),
            [order_id](const Order &order)
            {
                return order.id == order_id;
            });

        if (order_it != orders.end())
        {
            orders.erase(order_it);
        }

        if (orders.empty())
        {
            asks_.erase(price_level_it);
        }
    }

    order_location_.erase(order_id);
    return true;
}

bool OrderBook::has_bid() const
{
    return !bids_.empty();
}

bool OrderBook::has_ask() const
{
    return !asks_.empty();
}

double OrderBook::best_bid() const
{
    if (!has_bid())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return bids_.begin()->first;
}

double OrderBook::best_ask() const
{
    if (!has_ask())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return asks_.begin()->first;
}

double OrderBook::mid_price() const
{
    if (!has_bid() || !has_ask())
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return 0.5 * (best_bid() + best_ask());
}

int OrderBook::total_bid_depth() const
{
    return total_depth_on_side(Side::Buy);
}

int OrderBook::total_ask_depth() const
{
    return total_depth_on_side(Side::Sell);
}

int OrderBook::total_depth_on_side(Side side) const
{
    int depth = 0;

    if (side == Side::Buy)
    {
        for (const auto &price_level : bids_)
        {
            for (const auto &order : price_level.second)
            {
                depth += order.quantity;
            }
        }
    }
    else
    {
        for (const auto &price_level : asks_)
        {
            for (const auto &order : price_level.second)
            {
                depth += order.quantity;
            }
        }
    }

    return depth;
}

void OrderBook::remove_empty_price_levels()
{
    for (auto it = bids_.begin(); it != bids_.end();)
    {
        if (it->second.empty())
        {
            it = bids_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = asks_.begin(); it != asks_.end();)
    {
        if (it->second.empty())
        {
            it = asks_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
