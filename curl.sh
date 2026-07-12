#!/usr/bin/env bash
set -euo pipefail

BASE_URL="http://localhost:8080"
API_URL="$BASE_URL/api/matching"

# GET /api/matching/orders
# Returns the current resting orders in numeric side format:
#   side = 1 => BUY
#   side = 0 => SELL
curl -sS "$API_URL/orders"
printf '\n'
