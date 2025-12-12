#!/usr/bin/env bash
# Internal script called by CMake to run sonar-scanner with gosu if needed

set -euo pipefail

SCANNER="$1"
HOST_URL="$2"
PROJECT_KEY="$3"
SOURCE_DIR="$4"
GOSU="${5:-}"
SONAR_TOKEN="${SONAR_TOKEN:-}"

# Warn if SONAR_TOKEN is not set
if [ -z "$SONAR_TOKEN" ]; then
    echo "WARNING: SONAR_TOKEN environment variable is not set"
    echo "Analysis may fail with authentication error"
    echo "Set it with: export SONAR_TOKEN=squ_your_token_here"
fi

# Function to check if SonarQube server is running
check_sonar_server() {
    curl -s -o /dev/null -w "%{http_code}" "$HOST_URL/api/system/status" 2>/dev/null
}

# Function to start SonarQube server as sonaruser
start_sonar_server() {
    echo "SonarQube server not running. Starting..."

    # Find sonar.sh script
    SONAR_SH=""
    for path in /usr/local/bin/sonar.sh /opt/sonarqube/bin/linux-*/sonar.sh /opt/sonar/bin/linux-*/sonar.sh; do
        if [ -f "$path" ]; then
            SONAR_SH="$path"
            break
        fi
    done

    if [ -z "$SONAR_SH" ]; then
        echo "ERROR: sonar.sh not found"
        exit 1
    fi

    echo "Starting SonarQube with: $SONAR_SH"
    "$GOSU" sonaruser "$SONAR_SH" start

    # Wait for server to be ready (up to 90 seconds)
    echo "Waiting for SonarQube to be ready..."
    for i in {1..30}; do
        sleep 3
        STATUS=$(check_sonar_server)
        if [ "$STATUS" = "200" ]; then
            echo "SonarQube is ready!"
            return 0
        fi
        echo -n "."
    done
    echo ""
    echo "ERROR: SonarQube failed to start within 90 seconds"
    exit 1
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
    if [ -z "$GOSU" ]; then
        echo "ERROR: Running as root but gosu not available"
        exit 1
    fi

    # Verify sonaruser exists
    if ! id -u sonaruser >/dev/null 2>&1; then
        echo "ERROR: sonaruser does not exist (should be created in Docker image)"
        exit 1
    fi

    # Check if SonarQube server is running
    STATUS=$(check_sonar_server)
    if [ "$STATUS" != "200" ]; then
        start_sonar_server
    else
        echo "SonarQube server is running"
    fi

    # Save original ownership
    ORIGINAL_OWNER=$(stat -c '%U' "$SOURCE_DIR" 2>/dev/null || stat -f '%Su' "$SOURCE_DIR" 2>/dev/null || echo "root")
    ORIGINAL_GROUP=$(stat -c '%G' "$SOURCE_DIR" 2>/dev/null || stat -f '%Sg' "$SOURCE_DIR" 2>/dev/null || echo "root")

    # Ensure sonaruser owns the project directory
    chown -R sonaruser:sonaruser "$SOURCE_DIR"

    # Run as sonaruser and restore ownership afterward
    echo "Dropping privileges to sonaruser..."
    cd "$SOURCE_DIR"

    # Run scanner as sonaruser (preserve SONAR_TOKEN environment variable)
    "$GOSU" sonaruser env SONAR_TOKEN="$SONAR_TOKEN" "$SCANNER" \
        -Dsonar.host.url="$HOST_URL" \
        -Dsonar.projectKey="$PROJECT_KEY"

    EXIT_CODE=$?

    # Restore original ownership
    echo "Restoring ownership to $ORIGINAL_OWNER:$ORIGINAL_GROUP..."
    chown -R "$ORIGINAL_OWNER:$ORIGINAL_GROUP" "$SOURCE_DIR"

    exit $EXIT_CODE
else
    # Already non-root, run directly
    cd "$SOURCE_DIR"
    exec "$SCANNER" \
        -Dsonar.host.url="$HOST_URL" \
        -Dsonar.projectKey="$PROJECT_KEY"
fi
