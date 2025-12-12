#!/usr/bin/env bash
# Generate a SonarQube authentication token via API

set -euo pipefail

# Configuration
SONAR_HOST_URL="${SONAR_HOST_URL:-http://localhost:9000}"
SONAR_USER="${SONAR_USER:-admin}"
SONAR_PASSWORD="${SONAR_PASSWORD:-admin}"
TOKEN_NAME="${SONAR_TOKEN_NAME:-hm11-analysis-$(date +%Y%m%d-%H%M%S)}"

echo "Generating SonarQube token..."
echo "Server: $SONAR_HOST_URL"
echo "User: $SONAR_USER"
echo "Token name: $TOKEN_NAME"
echo ""

# Generate token using API
RESPONSE=$(curl -s -u "$SONAR_USER:$SONAR_PASSWORD" \
    -X POST \
    "$SONAR_HOST_URL/api/user_tokens/generate?name=$TOKEN_NAME")

# Check if successful
if echo "$RESPONSE" | grep -q '"token"'; then
    # Extract token from JSON response
    TOKEN=$(echo "$RESPONSE" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)

    echo "✓ Token generated successfully!"
    echo ""
    echo "Token: $TOKEN"
    echo ""
    echo "Export it with:"
    echo "  export SONAR_TOKEN=$TOKEN"
    echo ""
    echo "Or add to your shell profile (~/.bashrc or ~/.zshrc):"
    echo "  echo 'export SONAR_TOKEN=$TOKEN' >> ~/.bashrc"
else
    echo "✗ Failed to generate token"
    echo ""
    echo "Response: $RESPONSE"
    echo ""
    echo "Common issues:"
    echo "  1. Wrong credentials (default: admin/admin)"
    echo "  2. SonarQube server not running"
    echo "  3. First-time login requires password change via web UI"
    echo ""
    echo "Try logging in via web UI first: $SONAR_HOST_URL"
    exit 1
fi
