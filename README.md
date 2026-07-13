# java21-JNI

## Curl Example

`api_matching_orders_get.sh` fetches the current resting orders.
`api_matching_orders_post.sh` submits one sample order.

```bash
./api_matching_orders_get.sh
./api_matching_orders_post.sh
```

Response fields:

- `orderId`: unique order identifier
- `side`: `1` for `BUY`, `0` for `SELL`
- `price`: limit price
- `quantity`: order quantity
