#!/usr/bin/env bash

set -e

./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install sqlite3 glog hiredis grpc
