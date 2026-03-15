#!/usr/bin/env bash
# PriceBell code signing helper
# Usage: ./scripts/sign.sh <platform> <file>
# Requires appropriate environment variables to be set

set -euo pipefail

PLATFORM="${1:-}"
FILE="${2:-}"

case "$PLATFORM" in
    windows)
        if [ -z "${WINDOWS_CERT_BASE64:-}" ]; then
            echo "WINDOWS_CERT_BASE64 not set, skipping signing"
            exit 0
        fi
        echo "$WINDOWS_CERT_BASE64" | base64 -d > /tmp/cert.pfx
        signtool sign /f /tmp/cert.pfx /p "${WINDOWS_CERT_PASSWORD}" \
            /tr http://timestamp.digicert.com /td sha256 /fd sha256 "$FILE"
        rm -f /tmp/cert.pfx
        ;;
    linux)
        if [ -z "${GPG_PRIVATE_KEY:-}" ]; then
            echo "GPG_PRIVATE_KEY not set, skipping signing"
            exit 0
        fi
        echo "$GPG_PRIVATE_KEY" | gpg --batch --import
        gpg --batch --detach-sign --armor "$FILE"
        ;;
    *)
        echo "Usage: $0 <windows|linux> <file>"
        exit 1
        ;;
esac
