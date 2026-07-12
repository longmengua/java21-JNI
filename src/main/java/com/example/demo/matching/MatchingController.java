package com.example.demo.matching;

import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.ExceptionHandler;
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

    public MatchingController(NativeMatchingEngine engine) {
        this.engine = engine;
    }

    @PostMapping("/orders")
    public List<NativeMatchingEngine.Trade> submit(@RequestBody OrderRequest request) {
        return engine.submit(
                request.orderId(),
                request.side(),
                request.price(),
                request.quantity()
        );
    }

    @DeleteMapping("/orders/{orderId}")
    public CancelResponse cancel(@PathVariable long orderId) {
        return new CancelResponse(orderId, engine.cancel(orderId));
    }

    @ExceptionHandler({IllegalArgumentException.class, NullPointerException.class})
    @ResponseStatus(HttpStatus.BAD_REQUEST)
    public Map<String, String> badRequest(RuntimeException exception) {
        return Map.of("error", exception.getMessage());
    }

    public record OrderRequest(
            long orderId,
            NativeMatchingEngine.Side side,
            long price,
            long quantity
    ) {
    }

    public record CancelResponse(long orderId, boolean cancelled) {
    }
}
