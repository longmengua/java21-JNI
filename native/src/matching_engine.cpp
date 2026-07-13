#include "matching_engine.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>

namespace matching {

// 下單流程：
// 1. 驗證輸入
// 2. 檢查 orderId 是否重複
// 3. 依 side 撮合對手單
// 4. 剩餘量掛回 book
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

// 撤單流程：
// 1. 透過 orderId 找到定位資訊
// 2. 從對應 price level 的 queue 直接刪除
// 3. 如果該 price level 空了，就連整個 level 一起刪掉
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

// 這裡回傳的是 book 的 snapshot，不會修改任何狀態。
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

// 基本欄位驗證：這個 demo 只接受正數 id / price / quantity。
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

// Buy 單撮合邏輯：
// 先吃最便宜的賣單，若價格夠高就一直往下撮合。
void MatchingEngine::matchBuy(Order& incoming, std::vector<Trade>& trades) {
    while (incoming.quantity > 0 && !asks_.empty()) {
        // asks_ 已經由低到高排序，所以 begin() 就是最佳賣價。
        auto levelIt = asks_.begin();
        if (incoming.price < levelIt->first) {
            break;
        }

        auto& queue = levelIt->second;
        while (incoming.quantity > 0 && !queue.empty()) {
            // 同價位內用 FIFO，先處理 queue front。
            auto& maker = queue.front();
            const auto tradedQuantity = std::min(incoming.quantity, maker.quantity);

            // 記錄一筆成交：maker 是原本掛在 book 裡的單，taker 是新進來的單。
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
                // maker 完全成交後，從索引表移除並丟出 queue。
                activeOrders_.erase(maker.orderId);
                queue.pop_front();
            }
        }

        if (queue.empty()) {
            // 這個 price level 已經沒有任何單，就把整層刪掉。
            asks_.erase(levelIt);
        }
    }
}

// Sell 單撮合邏輯：
// 先吃最高價的買單，若價格夠低就一直往下撮合。
void MatchingEngine::matchSell(Order& incoming, std::vector<Trade>& trades) {
    while (incoming.quantity > 0 && !bids_.empty()) {
        // bids_ 用 greater<> 排序，所以 begin() 就是最佳買價。
        auto levelIt = bids_.begin();
        if (incoming.price > levelIt->first) {
            break;
        }

        auto& queue = levelIt->second;
        while (incoming.quantity > 0 && !queue.empty()) {
            // 同價位內仍然維持 FIFO。
            auto& maker = queue.front();
            const auto tradedQuantity = std::min(incoming.quantity, maker.quantity);

            // 產生成交記錄。
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
                // 完全成交後移除 maker。
                activeOrders_.erase(maker.orderId);
                queue.pop_front();
            }
        }

        if (queue.empty()) {
            // 這個買價層空了，整層刪除。
            bids_.erase(levelIt);
        }
    }
}

// 把剩餘的未成交訂單掛回 book，並記錄索引方便之後撤單。
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
