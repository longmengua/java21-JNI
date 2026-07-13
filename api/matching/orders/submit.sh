#!/usr/bin/env bash
set -euo pipefail

BASE_URL="http://localhost:8080"
API_URL="$BASE_URL/api/matching"

# POST /api/matching/orders
# side mapping:
#   1 = BUY
#   0 = SELL
#
# Edit these values directly when you want a different order.
ORDER_ID=1003
SIDE=0
PRICE=100
QUANTITY=1

curl -sS -X POST "$API_URL/orders" \
  -H 'Content-Type: application/json' \
  -d "{
    \"orderId\": $ORDER_ID,
    \"side\": $SIDE,
    \"price\": $PRICE,
    \"quantity\": $QUANTITY
  }"
printf '\n'
