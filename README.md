# java21-JNI

## Curl Example

`api/matching/orders/list.sh` fetches the current resting orders.
`api/matching/orders/submit.sh` submits one sample order.

```bash
./api/matching/orders/list.sh
./api/matching/orders/submit.sh
```

Response fields:

- `orderId`: unique order identifier
- `side`: `1` for `BUY`, `0` for `SELL`
- `price`: limit price
- `quantity`: order quantity

## Native Library

`matching.engine.library` is an optional JVM system property.

If you want to point the app at a specific JNI library file, start Java with:

```bash
-Dmatching.engine.library=/absolute/path/to/libmatching_engine.dylib
```

If it is not set, the app tries these locations in order:

1. `target/native/lib/<platform library name>`
2. `System.loadLibrary("matching_engine")`
