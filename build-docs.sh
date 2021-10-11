#!/bin/bash

set -euo pipefail

git worktree add doc/gh-pages gh-pages

mkdir -p build && cd build

# Configure
cmake -DPERCEMON_DOCS=ON -DCMAKE_BUILD_TYPE=Debug ..
# Build and Update the docs
make percemon-gh-pages.update

cd -
cd doc/gh-pages
git push origin gh-pages
cd -
git worktree remove doc/gh-pages

