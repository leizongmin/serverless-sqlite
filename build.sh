#!/usr/bin/env bash

set -e

dir=$(cd $(dirname "$0"); pwd)
echo "current dir: $dir"
cd "$dir"

target_dir="$dir/target"
echo "target dir:  $target_dir"

vcpkg_dir="$dir/vcpkg"
echo "vcpkg dir:   $vcpkg_dir"
vcpkg="$vcpkg_dir/vcpkg"
echo "vcpkg exec:  $vcpkg"

find src/ -iname *.hpp -o -iname *.cpp | xargs clang-format -style=file -i

cmake -B "$target_dir" -S . -DCMAKE_BUILD_TYPE=Release
cd "$target_dir"
make
