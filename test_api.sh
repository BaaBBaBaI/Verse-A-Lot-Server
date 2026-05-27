#!/bin/bash

# Configuration
API_URL="http://localhost:8080"
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color
BLUE='\033[0;34m'

echo -e "${BLUE}=== Starting API Verification Tests ===${NC}"

# Helper to print responses
print_response() {
    local status=$1
    local body=$2
    if [ "$status" -ge 200 ] && [ "$status" -lt 300 ]; then
        echo -e "${GREEN}SUCCESS (HTTP $status)${NC}"
        echo "$body" | jq . 2>/dev/null || echo "$body"
    else
        echo -e "${RED}FAILED (HTTP $status)${NC}"
        echo "$body" | jq . 2>/dev/null || echo "$body"
    fi
    echo ""
}

# --- Module 1: Auth & Users ---

# 1. Login Admin
echo -e "${BLUE}[TEST] Login Admin (POST /api/auth/login)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "admin123"}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"
ADMIN_TOKEN=$(echo "$BODY" | jq -r '.token')

# 2. Login User (winner)
echo -e "${BLUE}[TEST] Login User auction_winner (POST /api/auth/login)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "auction_winner", "password": "password123"}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"
USER_TOKEN=$(echo "$BODY" | jq -r '.token')

# 3. Get profile of user
echo -e "${BLUE}[TEST] Get Profile (GET /api/users/profile)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/users/profile" \
  -H "Authorization: Bearer $USER_TOKEN")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 4. List all users (Admin panel)
echo -e "${BLUE}[TEST] Get Users List as Admin (GET /api/admin/users)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/admin/users" \
  -H "Authorization: Bearer $ADMIN_TOKEN")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"


# --- Module 2: Auctions & Lots ---

# 5. List Auctions
echo -e "${BLUE}[TEST] Get Auctions List (GET /api/auctions)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/auctions")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 6. Create Auction as Admin
echo -e "${BLUE}[TEST] Create Auction (POST /api/admin/auctions)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/admin/auctions" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"title": "Ретро Аукцион Автомобилей Часть II", "type": "offline", "start_time": "2026-06-02 12:00:00", "end_time": "2026-06-02 18:00:00"}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 7. List Lots
echo -e "${BLUE}[TEST] Get Lots List (GET /api/lots)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/lots")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 8. Create a new Lot as User
echo -e "${BLUE}[TEST] Create Lot (POST /api/lots)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/lots" \
  -H "Authorization: Bearer $USER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"auction_id": "58a9c9ef-7689-4839-ac70-874f034cf92e", "title": "Антикварный серебряный кубок", "description": "Франция, конец 18-го века, проба 925.", "start_price": 3000.00, "reserve_price": 4500.00}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"
NEW_LOT_ID=$(echo "$BODY" | jq -r '.lot_id')

# 9. Moderate Lot as Admin
echo -e "${BLUE}[TEST] Moderate Lot (PUT /api/admin/lots/$NEW_LOT_ID/moderate)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X PUT "$API_URL/api/admin/lots/$NEW_LOT_ID/moderate" \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"status": "approved"}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"


# --- Module 3: Bids ---

# 10. Place a Bid
# Note: winner is placing a bid on painting lot 77a8c9ef-7689-4839-ac70-874f034cf92e
echo -e "${BLUE}[TEST] Place a Bid (POST /api/bids)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/bids" \
  -H "Authorization: Bearer $USER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"lot_id": "77a8c9ef-7689-4839-ac70-874f034cf92e", "amount": 6000.00}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 11. Get Bid History
echo -e "${BLUE}[TEST] Get Bid History (GET /api/lots/77a8c9ef-7689-4839-ac70-874f034cf92e/bids/history)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/lots/77a8c9ef-7689-4839-ac70-874f034cf92e/bids/history")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"


# --- Module 4: Transactions ---

# 12. Deposit Balance
echo -e "${BLUE}[TEST] Deposit Balance (POST /api/transactions/deposit)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST "$API_URL/api/transactions/deposit" \
  -H "Authorization: Bearer $USER_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"amount": 5000.00}')
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 13. Get My Transactions
echo -e "${BLUE}[TEST] Get My Transactions (GET /api/transactions/my)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/transactions/my" \
  -H "Authorization: Bearer $USER_TOKEN")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"

# 14. Get All Transactions as Admin
echo -e "${BLUE}[TEST] Get All Transactions List as Admin (GET /api/admin/transactions)${NC}"
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET "$API_URL/api/admin/transactions" \
  -H "Authorization: Bearer $ADMIN_TOKEN")
HTTP_STATUS=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')
print_response "$HTTP_STATUS" "$BODY"
