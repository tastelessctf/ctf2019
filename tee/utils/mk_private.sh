#/bin/sh
TARGET_DIR="./out-br/target"
DEPLOY_DIR="/home/nsr/tasteless/tctf19/tctf19/challenges/tee/private/"

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 flag1 flag2"
    exit
fi

echo "$1" > $TARGET_DIR/flag1.txt
echo "$2" > $TARGET_DIR/flag2.txt

(cd build && make)
cp out/bin/* $DEPLOY_DIR/deploy/qemu

echo "$1" > $DEPLOY_DIR/flag1.txt
echo "$2" > $DEPLOY_DIR/flag2.txt
