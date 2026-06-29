#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include "Order.h"

#include <deque>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

/*
OrderBook is a simple price-time priority limit order book.

For the bid side:
    Highest price has priority.
    Earlier orders at the same price have priority.

and ask side:
    Lowest price has priority.
    Earlier orders at the same price have priority.
*/

class OrderBook
{
public:
    OrderBook();

    // Main entry point for processing orders.
    std::vector<Trade> process_order(const Order &order);

    // Book state.
    bool has_bid() const;
    bool has_ask() const;

    double best_bid() const;
    double best_ask() const;
    double mid_price() const;

    int total_bid_depth() const;
    int total_ask_depth() const;

private:
    using BidBook = std::map<double, std::deque<Order>, std::greater<double>>;
    using AskBook = std::map<double, std::deque<Order>>;

    BidBook bids_;
    AskBook asks_;

    // Maps order_id to side/price so cancellations can find the order quickly.
    std::unordered_map<int, std::pair<Side, double>> order_location_;

    std::vector<Trade> process_limit_order(Order order);
    std::vector<Trade> process_market_order(Order order);
    bool cancel_order(int order_id);

    std::vector<Trade> match_buy_order(Order &incoming_order);
    std::vector<Trade> match_sell_order(Order &incoming_order);

    void add_resting_order(const Order &order);
    void remove_empty_price_levels();

    int total_depth_on_side(Side side) const;
};

#endif
