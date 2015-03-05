#!/bin/bash

# Strict mode
set -e

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
TEST_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

echo "################# Testing Go ##################"
rm -rf $TEST_DIR/go-build 
# TODO: go-ethereum needs to integrate ethash for this to work
#export GOPATH=$TEST_DIR/../go-build 
export GOPATH=$HOME/.go
export PATH=$PATH:$GOPATH/bin 
go get -d 
#go get -v github.com/ethereum/ethash 
go test
