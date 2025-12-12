#!/usr/bin/env bash
# Download SonarQube analysis report

set -euo pipefail

# Configuration
SONAR_HOST_URL="${SONAR_HOST_URL:-http://localhost:9000}"
SONAR_PROJECT_KEY="${SONAR_PROJECT_KEY:-hm11-driver}"
SONAR_TOKEN="${SONAR_TOKEN:-}"
OUTPUT_DIR="${SONAR_REPORT_DIR:-./sonar-reports}"

echo "Fetching SonarQube report..."
echo "Server: $SONAR_HOST_URL"
echo "Project: $SONAR_PROJECT_KEY"
echo "Output: $OUTPUT_DIR"
echo ""

# Check if token is set
if [ -z "$SONAR_TOKEN" ]; then
    echo "ERROR: SONAR_TOKEN not set"
    echo "Export it with: export SONAR_TOKEN=squ_your_token_here"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Get project status and metrics
echo "Fetching project metrics..."
curl -s -u "$SONAR_TOKEN:" \
    "$SONAR_HOST_URL/api/measures/component?component=$SONAR_PROJECT_KEY&metricKeys=bugs,vulnerabilities,code_smells,coverage,duplicated_lines_density,ncloc,sqale_rating,reliability_rating,security_rating" \
    | python3 -m json.tool > "$OUTPUT_DIR/metrics.json"

# Get issues (bugs, vulnerabilities, code smells)
echo "Fetching issues..."
curl -s -u "$SONAR_TOKEN:" \
    "$SONAR_HOST_URL/api/issues/search?componentKeys=$SONAR_PROJECT_KEY&ps=500" \
    | python3 -m json.tool > "$OUTPUT_DIR/issues.json"

# Get quality gate status
echo "Fetching quality gate status..."
curl -s -u "$SONAR_TOKEN:" \
    "$SONAR_HOST_URL/api/qualitygates/project_status?projectKey=$SONAR_PROJECT_KEY" \
    | python3 -m json.tool > "$OUTPUT_DIR/quality_gate.json"

# Generate summary report
echo "Generating summary report..."
python3 - <<'PYTHON_SCRIPT' "$OUTPUT_DIR"
import json
import sys
from pathlib import Path

output_dir = Path(sys.argv[1])

# Load metrics
with open(output_dir / 'metrics.json') as f:
    metrics_data = json.load(f)

# Load quality gate
with open(output_dir / 'quality_gate.json') as f:
    qg_data = json.load(f)

# Load issues
with open(output_dir / 'issues.json') as f:
    issues_data = json.load(f)

# Extract metrics
metrics = {}
for measure in metrics_data.get('component', {}).get('measures', []):
    metrics[measure['metric']] = measure.get('value', 'N/A')

# Generate summary
summary = f"""# SonarQube Analysis Summary

**Project**: {metrics_data.get('component', {}).get('key', 'N/A')}
**Date**: {metrics_data.get('component', {}).get('analysisDate', 'N/A')}

## Quality Gate
**Status**: {qg_data.get('projectStatus', {}).get('status', 'N/A')}

## Metrics

### Reliability
- **Bugs**: {metrics.get('bugs', 'N/A')}
- **Reliability Rating**: {metrics.get('reliability_rating', 'N/A')}

### Security
- **Vulnerabilities**: {metrics.get('vulnerabilities', 'N/A')}
- **Security Rating**: {metrics.get('security_rating', 'N/A')}

### Maintainability
- **Code Smells**: {metrics.get('code_smells', 'N/A')}
- **Technical Debt Rating**: {metrics.get('sqale_rating', 'N/A')}

### Coverage
- **Coverage**: {metrics.get('coverage', 'N/A')}%

### Duplication
- **Duplicated Lines**: {metrics.get('duplicated_lines_density', 'N/A')}%

### Size
- **Lines of Code**: {metrics.get('ncloc', 'N/A')}

## Issues Summary
- **Total Issues**: {issues_data.get('total', 0)}
- **Blocker**: {sum(1 for i in issues_data.get('issues', []) if i.get('severity') == 'BLOCKER')}
- **Critical**: {sum(1 for i in issues_data.get('issues', []) if i.get('severity') == 'CRITICAL')}
- **Major**: {sum(1 for i in issues_data.get('issues', []) if i.get('severity') == 'MAJOR')}
- **Minor**: {sum(1 for i in issues_data.get('issues', []) if i.get('severity') == 'MINOR')}
- **Info**: {sum(1 for i in issues_data.get('issues', []) if i.get('severity') == 'INFO')}

## View Full Report
{qg_data.get('projectStatus', {}).get('projectUrl', 'N/A')}
"""

# Write summary
with open(output_dir / 'summary.md', 'w') as f:
    f.write(summary)

print(summary)
PYTHON_SCRIPT

echo ""
echo "✓ Report generated successfully!"
echo ""
echo "Files created in $OUTPUT_DIR:"
echo "  - metrics.json       (raw metrics data)"
echo "  - issues.json        (all issues)"
echo "  - quality_gate.json  (quality gate status)"
echo "  - summary.md         (human-readable summary)"
echo ""
echo "View summary:"
echo "  cat $OUTPUT_DIR/summary.md"
