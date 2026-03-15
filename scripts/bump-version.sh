#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>  (e.g., 2.1.0)"
    exit 1
fi

VERSION="$1"
TAG="v${VERSION}"

# Validate semver format
if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: Version must be in X.Y.Z format"
    exit 1
fi

# Check for clean working tree
if ! git diff --quiet HEAD; then
    echo "Error: Working tree is dirty. Commit or stash changes first."
    exit 1
fi

# Update version in CMakeLists.txt
sed -i "s/project(PriceBell VERSION [0-9]\+\.[0-9]\+\.[0-9]\+/project(PriceBell VERSION ${VERSION}/" CMakeLists.txt

git add CMakeLists.txt
git commit -m "chore: bump version to ${VERSION}"
git tag -a "${TAG}" -m "Release ${TAG}"

echo ""
echo "Version bumped to ${VERSION}"
echo "Tag ${TAG} created"
echo ""
echo "To trigger the release workflow, push with:"
echo "  git push origin main ${TAG}"
