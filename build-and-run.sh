#!/usr/bin/env bash

set -e

./build.sh
echo "--------------------"
cd target
GLOG_logtostderr=1 ./query
echo "--------------------"
