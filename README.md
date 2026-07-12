# java21-JNI

## Curl Example

`curl.sh` sends one sample order to the matching API.

```bash
./curl.sh
```

Request fields:

- `orderId`: unique order identifier
- `side`: `1` for `BUY`, `0` for `SELL`
- `price`: limit price
- `quantity`: order quantity
