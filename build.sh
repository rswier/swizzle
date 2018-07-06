DEBUG=""

if [ "$1" = "-d" ]; then
    DEBUG="-D DEBUG=1"
fi

clang -O3 -o swizzle swizzle.c -ldl $DEBUG
