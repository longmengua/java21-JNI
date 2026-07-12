#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace matching {

enum class Side : std::int32_t {
    Buy = 0,
    Sell = 1
};

struct Order {
    std::int64_t orderId;
    Side side;
    std::int64_t price;
    std::int64_t quantity;
};

struct Trade {
    std::int64_t tradeId;
    std::int64_t makerOrderId;
    std::int64_t takerOrderId;
    std::int64_t price;
    std::int64_t quantity;
};

class MatchingEngine final {
public:
    std::vector<Trade> submit(
        std::int64_t orderId,
        Side side,
        std::int64_t price,
        std::int64_t quantity
    );

    bool cancel(std::int64_t orderId);

private:
    using OrderQueue = std::list<Order>;
    using OrderIterator = OrderQueue::iterator;

    struct Locator {
        Side side;
        std::int64_t price;
        OrderIterator iterator;
    };

    std::map<std::int64_t, OrderQueue, std::greater<>> bids_;
    std::map<std::int64_t, OrderQueue> asks_;
    std::unordered_map<std::int64_t, Locator> activeOrders_;
    std::unordered_set<std::int64_t> seenOrderIds_;
    std::int64_t nextTradeId_ = 1;
    std::mutex mutex_;

    static void validateOrder(
        std::int64_t orderId,
        std::int64_t price,
        std::int64_t quantity
    );

    void matchBuy(Order& incoming, std::vector<Trade>& trades);
    void matchSell(Order& incoming, std::vector<Trade>& trades);
    void addRestingOrder(const Order& order);
};

} // namespace matching
