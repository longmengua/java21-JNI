package com.example.demo.matching;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

@RestController
@RequestMapping("/api/matching")
public final class MatchingController {

    private final NativeMatchingEngine engine;

    // Controller 只負責把 HTTP request 轉給 native engine。
    public MatchingController(NativeMatchingEngine engine) {
        this.engine = engine;
    }

    // 送出新訂單，回傳立即撮合出的成交結果。
    @PostMapping("/orders")
    public List<NativeMatchingEngine.Trade> submit(@RequestBody OrderRequest request) {
        return engine.submit(
                request.orderId(),
                toSide(request.side()),
                request.price(),
                request.quantity()
        );
    }

    // 透過 order ID 取消尚未成交的掛單。
    @DeleteMapping("/orders/{orderId}")
    public CancelResponse cancel(@PathVariable long orderId) {
        return new CancelResponse(orderId, engine.cancel(orderId));
    }

    // 查詢目前所有尚未成交的掛單。
    @GetMapping("/orders")
    public List<NativeMatchingEngine.OpenOrder> orders() {
        return engine.listOrders();
    }

    // 把常見輸入錯誤轉成 400 回應。
    @ExceptionHandler({IllegalArgumentException.class, NullPointerException.class})
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public Map<String, String> badRequest(RuntimeException exception) {
        return Map.of("error", exception.getMessage());
    }

    // POST /api/matching/orders 的 request body。
    public record OrderRequest(
            long orderId,
            int side,
            long price,
            long quantity
    ) {
    }

    // DELETE /api/matching/orders/{orderId} 的 response body。
    public record CancelResponse(long orderId, boolean cancelled) {
    }

    // 對外 API 約定：1 代表 BUY，0 代表 SELL。
    private static NativeMatchingEngine.Side toSide(int side) {
        return switch (side) {
            case 1 -> NativeMatchingEngine.Side.BUY;
            case 0 -> NativeMatchingEngine.Side.SELL;
            default -> throw new IllegalArgumentException("side must be 1 for BUY or 0 for SELL");
        };
    }
}
