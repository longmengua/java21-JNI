package com.example.demo.matching;

import jakarta.annotation.PreDestroy;
import org.springframework.stereotype.Component;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

@Component
public final class NativeMatchingEngine implements AutoCloseable {

    // Spring bean 載入時先初始化 native library。
    static {
        loadNativeLibrary();
    }

    // 指向 C++ 端 MatchingEngine 的不透明 handle。
    private long handle = nativeCreate();

    // 送出新訂單，並把 native 回傳的成交結果展平成 Java 物件。
    public synchronized List<Trade> submit(
            long orderId,
            Side side,
            long price,
            long quantity
    ) {
        // shutdown 時 handle 可能已被關閉，所以每個公開方法都先檢查。
        ensureOpen();
        Objects.requireNonNull(side, "side");

        // JNI 會回傳扁平化 long[]，每筆 trade 佔 5 個值。
        long[] values = nativeSubmit(
                handle,
                orderId,
                side.nativeValue,
                price,
                quantity
        );

        if (values == null || values.length == 0) {
            return List.of();
        }
        if (values.length % 5 != 0) {
            throw new IllegalStateException("invalid trade result returned by native engine");
        }

        List<Trade> trades = new ArrayList<>(values.length / 5);
        for (int index = 0; index < values.length; index += 5) {
            trades.add(new Trade(
                    values[index],
                    values[index + 1],
                    values[index + 2],
                    values[index + 3],
                    values[index + 4]
            ));
        }
        return List.copyOf(trades);
    }

    // 用 order ID 取消尚未成交的掛單。
    public synchronized boolean cancel(long orderId) {
        ensureOpen();
        return nativeCancel(handle, orderId);
    }

    // 取得目前所有尚未成交的掛單，並轉成 Java record。
    public synchronized List<OpenOrder> listOrders() {
        ensureOpen();

        // JNI 會回傳扁平化 long[]，每筆 order 佔 4 個值。
        long[] values = nativeListOrders(handle);
        if (values == null || values.length == 0) {
            return List.of();
        }
        if (values.length % 4 != 0) {
            throw new IllegalStateException("invalid order result returned by native engine");
        }

        List<OpenOrder> orders = new ArrayList<>(values.length / 4);
        for (int index = 0; index < values.length; index += 4) {
            orders.add(new OpenOrder(
                    values[index],
                    toSideValue(values[index + 1]),
                    values[index + 2],
                    values[index + 3]
            ));
        }
        return List.copyOf(orders);
    }

    // Spring 關閉 bean 時會呼叫這裡，釋放 native 資源。
    @PreDestroy
    @Override
    public synchronized void close() {
        if (handle != 0) {
            nativeDestroy(handle);
            handle = 0;
        }
    }

    // 如果 native engine 已經被關掉，就直接失敗，不再往下走。
    private void ensureOpen() {
        if (handle == 0) {
            throw new IllegalStateException("native matching engine is closed");
        }
    }

    // 依序從指定路徑、Maven build 產物、系統 library path 載入 JNI library。
    private static void loadNativeLibrary() {
        String configuredPath = System.getProperty("matching.engine.library");
        if (configuredPath != null && !configuredPath.isBlank()) {
            System.load(Path.of(configuredPath).toAbsolutePath().toString());
            return;
        }

        Path localBuild = Path.of(
                "target",
                "native",
                "lib",
                System.mapLibraryName("matching_engine")
        ).toAbsolutePath();

        if (Files.isRegularFile(localBuild)) {
            System.load(localBuild.toString());
            return;
        }

        System.loadLibrary("matching_engine");
    }

    // Java 端的 side 表示法，對應 native 的數字值。
    public enum Side {
        BUY(0),
        SELL(1);

        private final int nativeValue;

        Side(int nativeValue) {
            this.nativeValue = nativeValue;
        }

        int nativeValue() {
            return nativeValue;
        }

        // 驗證 JNI 只會回傳支援的兩種 side 值。
        static Side fromNativeValue(long nativeValue) {
            if (nativeValue == BUY.nativeValue) {
                return BUY;
            }
            if (nativeValue == SELL.nativeValue) {
                return SELL;
            }
            throw new IllegalArgumentException("side must be 0 or 1");
        }
    }

    // 只有真的撮合成交時，才會回傳 Trade。
    public record Trade(
            long tradeId,
            long makerOrderId,
            long takerOrderId,
            long price,
            long quantity
    ) {
    }

    // OpenOrder 代表目前還留在 book 裡的掛單。
    public record OpenOrder(
            long orderId,
            int side,
            long price,
            long quantity
    ) {
    }

    // native 物件生命週期。
    private static native long nativeCreate();

    private static native void nativeDestroy(long handle);

    // native submit 回傳扁平化成交資料：
    // [tradeId, makerOrderId, takerOrderId, price, quantity, ...]
    private static native long[] nativeSubmit(
            long handle,
            long orderId,
            int side,
            long price,
            long quantity
    );

    private static native boolean nativeCancel(long handle, long orderId);

    // native 查詢回傳扁平化掛單資料：
    // [orderId, side, price, quantity, ...]
    private static native long[] nativeListOrders(long handle);

    // 驗證 native side 值後，回傳 0/1 這個對外約定。
    private static int toSideValue(long nativeValue) {
        return Side.fromNativeValue(nativeValue).nativeValue();
    }
}
