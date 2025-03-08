#!/bin/bash

# Test script for validating theme JSON files
# This test checks that all theme files are valid JSON
# and conform to our expected structure

echo "Testing theme JSON files..."

# Check if jq is available (needed for JSON validation)
if ! command -v jq &> /dev/null; then
    echo "Warning: jq command not found. Installing JSON validation will be limited."
    HAS_JQ=0
else
    HAS_JQ=1
fi

# Directories to check
THEME_DIRS=(
    "./themes"
    "$HOME/.nutshell/themes"
)

# Schema validation function
validate_theme_schema() {
    local file="$1"
    local schema_errors=0
    
    # Basic checks that can be done with or without jq
    if grep -q '"segments"' "$file"; then
        echo "✓ Contains segments section"
    else
        echo "✗ Missing segments section"
        schema_errors=$((schema_errors+1))
    fi
    
    if grep -q '"commands"' "$file"; then
        echo "✓ Contains commands section (new format)"
    else
        echo "⚠ May be using old format (missing 'commands')"
    fi
    
    # More detailed validation with jq if available
    if [ "$HAS_JQ" -eq 1 ]; then
        # Check required top-level fields
        if ! jq -e '.name' "$file" > /dev/null 2>&1; then
            echo "✗ Missing required field: name"
            schema_errors=$((schema_errors+1))
        fi
        
        if ! jq -e '.prompt' "$file" > /dev/null 2>&1; then
            echo "✗ Missing required field: prompt"
            schema_errors=$((schema_errors+1))
        fi
        
        if ! jq -e '.colors' "$file" > /dev/null 2>&1; then
            echo "✗ Missing required field: colors"
            schema_errors=$((schema_errors+1))
        fi
        
        # Check that segments have commands objects
        if jq -e '.segments | keys[]' "$file" > /dev/null 2>&1; then
            for segment in $(jq -r '.segments | keys[]' "$file"); do
                if ! jq -e ".segments[\"$segment\"].commands" "$file" > /dev/null 2>&1; then
                    echo "⚠ Segment '$segment' is using old format (no commands object)"
                else
                    echo "✓ Segment '$segment' has commands object (new format)"
                fi
            done
        fi
    fi
    
    return $schema_errors
}

# Test results tracking
TOTAL=0
PASSED=0
FAILED=0

for dir in "${THEME_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        echo "Checking themes in $dir..."
        
        # Find all JSON files
        for theme_file in "$dir"/*.json; do
            if [ -f "$theme_file" ]; then
                TOTAL=$((TOTAL+1))
                filename=$(basename "$theme_file")
                echo "----------------------------------------"
                echo "Testing theme: $filename"
                
                # 1. Check if it's valid JSON
                if [ "$HAS_JQ" -eq 1 ]; then
                    if jq '.' "$theme_file" > /dev/null 2>&1; then
                        echo "✓ Valid JSON format"
                        
                        # 2. Validate theme schema
                        errors=$(validate_theme_schema "$theme_file")
                        if [ "$?" -eq 0 ]; then
                            echo "✓ Schema validation passed"
                            PASSED=$((PASSED+1))
                        else
                            echo "✗ Schema validation failed"
                            FAILED=$((FAILED+1))
                        fi
                    else
                        echo "✗ Invalid JSON format"
                        FAILED=$((FAILED+1))
                    fi
                else
                    # Fallback to simple validation
                    if grep -q "{" "$theme_file" && grep -q "}" "$theme_file"; then
                        echo "⚠ Appears to be JSON but cannot fully validate (jq not available)"
                        validate_theme_schema "$theme_file"
                        PASSED=$((PASSED+1))
                    else
                        echo "✗ Does not appear to be valid JSON"
                        FAILED=$((FAILED+1))
                    fi
                fi
            fi
        done
    fi
done

echo "----------------------------------------"
echo "Theme JSON tests summary:"
echo "Total themes tested: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"

if [ "$FAILED" -eq 0 ]; then
    echo "All theme tests passed!"
    exit 0
else
    echo "Some theme tests failed!"
    exit 1
fi
