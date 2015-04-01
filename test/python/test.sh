#!/bin/bash

# Strict mode
set -e

# just a specific case for archlinux where python3 is the default
if [ -f "/etc/arch-release" ]; then
    VIRTUALENV_EXEC=virtualenv2
else
    VIRTUALENV_EXEC=virtualenv
fi

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
TEST_DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

[ -d $TEST_DIR/python-virtual-env ] || $VIRTUALENV_EXEC --system-site-packages $TEST_DIR/python-virtual-env
source $TEST_DIR/python-virtual-env/bin/activate
pip install -r $TEST_DIR/requirements.txt > /dev/null
# force installation of nose in virtualenv even if existing in thereuser's system
pip install nose -I
pip install --upgrade --no-deps --force-reinstall -e $TEST_DIR/../..
cd $TEST_DIR
nosetests --with-doctest -v --nocapture
