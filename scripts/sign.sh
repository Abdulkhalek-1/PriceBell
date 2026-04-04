#!/usr/bin/env bash
# PriceBell code signing helper
# Usage: ./scripts/sign.sh <platform> <file>
#
# Self-signed cert mode (no external secrets):
#   ./scripts/sign.sh self-signed-cert   — generate cert if not present
#
# CI signing mode (requires env vars):
#   ./scripts/sign.sh windows <file>
#   ./scripts/sign.sh linux   <file>

set -euo pipefail

PLATFORM="${1:-}"
FILE="${2:-}"

CERT_DIR="${HOME}/.pricebell-signing"
CERT_PEM="${CERT_DIR}/pricebell-self.pem"
CERT_KEY="${CERT_DIR}/pricebell-self.key"
CERT_PFX="${CERT_DIR}/pricebell-self.pfx"

generate_self_signed() {
    mkdir -p "${CERT_DIR}"
    if [ -f "${CERT_PEM}" ]; then
        echo "Self-signed cert already exists at ${CERT_PEM}"
        return
    fi
    echo "Generating self-signed certificate..."
    openssl req -x509 -newkey rsa:4096 -days 3650 -nodes \
        -keyout "${CERT_KEY}" \
        -out    "${CERT_PEM}" \
        -subj "/CN=Abdulkhalek Muhammad/O=PriceBell/C=SA"
    openssl pkcs12 -export -passout pass: \
        -inkey "${CERT_KEY}" \
        -in    "${CERT_PEM}" \
        -out   "${CERT_PFX}"
    echo "Certificate generated at ${CERT_DIR}"
}

case "$PLATFORM" in
    self-signed-cert)
        generate_self_signed
        ;;
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
    windows-self-signed)
        generate_self_signed
        signtool sign /f "${CERT_PFX}" /p "" \
            /tr http://timestamp.digicert.com /td sha256 /fd sha256 "$FILE"
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
        echo "Usage: $0 <self-signed-cert|windows|windows-self-signed|linux> [file]"
        exit 1
        ;;
esac
