#include "matching_engine.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace matching {

std::vector<Trade> MatchingEngine::submit(
    const std::int64_t orderId,
    const Side side,
    const std::int64_t price,
    const std::int64_t quantity
) {
    std::lock_guard lock(mutex_);
    validateOrder(orderId, price, quantity);

    if (seenOrderIds_.find(orderId) != seenOrderIds_.end()) {
        throw std::invalid_argument("duplicate order ID");
    }
    seenOrderIds_.insert(orderId);

    Order incoming{orderId, side, price, quantity};
    std::vector<Trade> trades;

    if (side == Side::Buy) {
        matchBuy(incoming, trades);
    } else {
        matchSell(incoming, trades);
    }

    if (incoming.quantity > 0) {
        addRestingOrder(incoming);
    }
    return trades;
}

bool MatchingEngine::cancel(const std::int64_t orderId) {
    std::lock_guard lock(mutex_);
    const auto indexIt = activeOrders_.find(orderId);
    if (indexIt == activeOrders_.end()) {
        return false;
    }

    const Locator locator = indexIt->second;
    if (locator.side == Side::Buy) {
        const auto levelIt = bids_.find(locator.price);
        if (levelIt == bids_.end()) {
            throw std::logic_error("buy order index is inconsistent");
        }
        levelIt->second.erase(locator.iterator);
        if (levelIt->second.empty()) {
            bids_.erase(levelIt);
        }
    } else {
        const auto levelIt = asks_.find(locator.price);
        if (levelIt == asks_.end()) {
            throw std::logic_error("sell order index is inconsistent");
        }
        levelIt->second.erase(locator.iterator);
        if (levelIt->second.empty()) {
            asks_.erase(levelIt);
        }
    }

    activeOrders_.erase(indexIt);
    return true;
}

std::vector<Order> MatchingEngine::listOrders() {
    std::lock_guard lock(mutex_);

    std::vector<Order> orders;
    for (const auto& level : bids_) {
        const auto& queue = level.second;
        for (const auto& order : queue) {
            orders.push_back(order);
        }
    }
    for (const auto& level : asks_) {
        const auto& queue = level.second;
        for (const auto& order : queue) {
            orders.push_back(order);
        }
    }

    return orders;
}

void MatchingEngine::validateOrder(
    const std::int64_t orderId,
    const std::int64_t price,
    const std::int64_t quantity
) {
    if (orderId <= 0) {
        throw std::invalid_argument("orderId must be positive");
    }
    if (price <= 0) {
        throw std::invalid_argument("price must be positive");
    }
    if (quantity <= 0) {
        throw std::invalid_argument("quantity must be positive");
    }
}

void MatchingEngine::matchBuy(Order& incoming, std::vector<Trade>& trades) {
    while (incoming.quantity > 0 && !asks_.empty()) {
        auto levelIt = asks_.begin();
        if (incoming.price < levelIt->first) {
            break;
        }

        auto& queue = levelIt->second;
        while (incoming.quantity > 0 && !queue.empty()) {
            auto& maker = queue.front();
            const auto tradedQuantity = std::min(incoming.quantity, maker.quantity);

            trades.push_back({
                nextTradeId_++,
                maker.orderId,
                incoming.orderId,
                maker.price,
                tradedQuantity
            });

            incoming.quantity -= tradedQuantity;
            maker.quantity -= tradedQuantity;
            if (maker.quantity == 0) {
                activeOrders_.erase(maker.orderId);
                queue.pop_front();
            }
        }

        if (queue.empty()) {
            asks_.erase(levelIt);
        }
    }
}

void MatchingEngine::matchSell(Order& incoming, std::vector<Trade>& trades) {
    while (incoming.quantity > 0 && !bids_.empty()) {
        auto levelIt = bids_.begin();
        if (incoming.price > levelIt->first) {
            break;
        }

        auto& queue = levelIt->second;
        while (incoming.quantity > 0 && !queue.empty()) {
            auto& maker = queue.front();
            const auto tradedQuantity = std::min(incoming.quantity, maker.quantity);

            trades.push_back({
                nextTradeId_++,
                maker.orderId,
                incoming.orderId,
                maker.price,
                tradedQuantity
            });

            incoming.quantity -= tradedQuantity;
            maker.quantity -= tradedQuantity;
            if (maker.quantity == 0) {
                activeOrders_.erase(maker.orderId);
                queue.pop_front();
            }
        }

        if (queue.empty()) {
            bids_.erase(levelIt);
        }
    }
}

void MatchingEngine::addRestingOrder(const Order& order) {
    if (order.side == Side::Buy) {
        auto& queue = bids_[order.price];
        queue.push_back(order);
        activeOrders_.emplace(order.orderId, Locator{
            order.side,
            order.price,
            std::prev(queue.end())
        });
    } else {
        auto& queue = asks_[order.price];
        queue.push_back(order);
        activeOrders_.emplace(order.orderId, Locator{
            order.side,
            order.price,
            std::prev(queue.end())
        });
    }
}

} // namespace matching
