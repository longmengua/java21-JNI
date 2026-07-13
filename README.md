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
