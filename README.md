# java21-JNI

## Curl Example

`curl.sh` fetches the current resting orders from the matching API.

```bash
./curl.sh
```

Response fields:

- `orderId`: unique order identifier
- `side`: `1` for `BUY`, `0` for `SELL`
- `price`: limit price
- `quantity`: order quantity
