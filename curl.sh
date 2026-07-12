#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://localhost:8080}"
API_URL="${API_URL:-$BASE_URL/api/matching}"

usage() {
  cat <<'EOF'
Usage:
  ./curl.sh buy
  ./curl.sh sell
  ./curl.sh cancel <orderId>

Environment:
  BASE_URL=http://localhost:8080
  API_URL=http://localhost:8080/api/matching

Examples:
  ./curl.sh buy
  ./curl.sh sell
  ./curl.sh cancel 1001
EOF
}

submit_order() {
  local order_id="$1"
  local side="$2"
  local price="$3"
  local quantity="$4"

  curl -sS -X POST "$API_URL/orders" \
    -H 'Content-Type: application/json' \
    -d "{
      \"orderId\": $order_id,
      \"side\": \"$side\",
      \"price\": $price,
      \"quantity\": $quantity
    }"
  printf '\n'
}

cancel_order() {
  local order_id="$1"

  curl -sS -X DELETE "$API_URL/orders/$order_id"
  printf '\n'
}

if [ $# -lt 1 ]; then
  usage
  exit 1
fi

case "$1" in
  buy)
    submit_order 1001 BUY 101 10
    ;;
  sell)
    submit_order 1002 SELL 101 10
    ;;
  cancel)
    if [ $# -ne 2 ]; then
      usage
      exit 1
    fi
    cancel_order "$2"
    ;;
  *)
    usage
    exit 1
    ;;
esac
