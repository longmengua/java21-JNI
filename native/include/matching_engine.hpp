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

// side 的數字約定和 Java 一致：
// 0 代表 Buy，1 代表 Sell。
enum class Side : std::int32_t {
    Buy = 0,
    Sell = 1
};

// 一筆掛單的完整資料。
struct Order {
    std::int64_t orderId;
    Side side;
    std::int64_t price;
    std::int64_t quantity;
};

// 一次撮合產生的一筆成交。
struct Trade {
    std::int64_t tradeId;
    std::int64_t makerOrderId;
    std::int64_t takerOrderId;
    std::int64_t price;
    std::int64_t quantity;
};

// 撮合引擎本體，負責下單、撤單與查詢目前掛單。
class MatchingEngine final {
public:
    // 下單後可能產生成交，也可能剩餘部分掛在 book 裡。
    std::vector<Trade> submit(
        std::int64_t orderId,
        Side side,
        std::int64_t price,
        std::int64_t quantity
    );

    // 依 orderId 撤銷尚未成交的掛單。
    bool cancel(std::int64_t orderId);

    // 取出目前 book 裡所有尚未成交的掛單。
    std::vector<Order> listOrders();

private:
    // 同價位的掛單要維持先進先出，所以用 list 存 queue。
    using OrderQueue = std::list<Order>;
    using OrderIterator = OrderQueue::iterator;

    // 快速撤單用的定位資訊：知道 side、price、以及 queue 位置。
    struct Locator {
        Side side;
        std::int64_t price;
        OrderIterator iterator;
    };

    // 買單簿：價格越高越優先，所以用 greater<> 讓 map 由大到小排序。
    std::map<std::int64_t, OrderQueue, std::greater<>> bids_;
    // 賣單簿：價格越低越優先，所以維持一般由小到大的排序。
    std::map<std::int64_t, OrderQueue> asks_;
    // orderId -> 位置索引，撤單時可以 O(1) 找到目標。
    std::unordered_map<std::int64_t, Locator> activeOrders_;
    // 記錄曾經用過的 orderId，避免重複下單。
    std::unordered_set<std::int64_t> seenOrderIds_;
    // 成交編號，單調遞增。
    std::int64_t nextTradeId_ = 1;
    // 保護整個 book 的互斥鎖。
    std::mutex mutex_;

    // 驗證基本欄位是否合理。
    static void validateOrder(
        std::int64_t orderId,
        std::int64_t price,
        std::int64_t quantity
    );

    // buy 單往 asks 撮合。
    void matchBuy(Order& incoming, std::vector<Trade>& trades);
    // sell 單往 bids 撮合。
    void matchSell(Order& incoming, std::vector<Trade>& trades);
    // 尚未成交的剩餘量，會被掛回 book。
    void addRestingOrder(const Order& order);
};

} // namespace matching
