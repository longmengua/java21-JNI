#!/usr/bin/env bash
set -euo pipefail

BASE_URL="http://localhost:8080"
API_URL="$BASE_URL/api/matching"

# Edit these values directly when you want a different order.
ORDER_ID=1001
SIDE="BUY"
PRICE=101
QUANTITY=10

curl -sS -X POST "$API_URL/orders" \
  -H 'Content-Type: application/json' \
  -d "{
    \"orderId\": $ORDER_ID,
    \"side\": \"$SIDE\",
    \"price\": $PRICE,
    \"quantity\": $QUANTITY
  }"
printf '\n'
