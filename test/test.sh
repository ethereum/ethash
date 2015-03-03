#!/usr/bin/env bash -x

SOURCE="${BASH_SOURCE[0]}"

while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done

TEST_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

echo "################# Testing CPP ##################"

( rm -rf $TEST_DIR/../cpp-build ;
  mkdir -p $TEST_DIR/../cpp-build ;
  cd $TEST_DIR/../cpp-build ; 
  cmake .. ;
  make Test ;
  test/Test )

#echo "################# Testing Go ##################"
#( rm -rf $TEST_DIR/../go-build ;
#  export GOPATH=$TEST_DIR/../go-build ;
#  export PATH=$PATH:$GOPATH/bin ;
#  cd $TEST_DIR/go ; 
#  go get -dv ;
#  go get -v github.com/ethereum/ethash ;
#  go test )
