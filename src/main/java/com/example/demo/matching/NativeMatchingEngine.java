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

    static {
        loadNativeLibrary();
    }

    private long handle = nativeCreate();

    public synchronized List<Trade> submit(
            long orderId,
            Side side,
            long price,
            long quantity
    ) {
        ensureOpen();
        Objects.requireNonNull(side, "side");

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

    public synchronized boolean cancel(long orderId) {
        ensureOpen();
        return nativeCancel(handle, orderId);
    }

    @PreDestroy
    @Override
    public synchronized void close() {
        if (handle != 0) {
            nativeDestroy(handle);
            handle = 0;
        }
    }

    private void ensureOpen() {
        if (handle == 0) {
            throw new IllegalStateException("native matching engine is closed");
        }
    }

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

    public enum Side {
        BUY(0),
        SELL(1);

        private final int nativeValue;

        Side(int nativeValue) {
            this.nativeValue = nativeValue;
        }
    }

    public record Trade(
            long tradeId,
            long makerOrderId,
            long takerOrderId,
            long price,
            long quantity
    ) {
    }

    private static native long nativeCreate();

    private static native void nativeDestroy(long handle);

    private static native long[] nativeSubmit(
            long handle,
            long orderId,
            int side,
            long price,
            long quantity
    );

    private static native boolean nativeCancel(long handle, long orderId);
}
