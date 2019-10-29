#/bin/sh
TARGET_DIR="./out-br/target"

if [ "$#" -lt 2 ]; then
    echo "Usage: $0 fake_flag1 fake_flag2"
    exit
fi

echo "$1" > $TARGET_DIR/flag1.txt
echo "$2" > $TARGET_DIR/flag2.txt

(cd build && make)
exec ./cp2git.sh /home/nsr/tasteless/tctf19/tctf19/challenges/tee/public/

