#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Wrong usage: ./config.sh [board name] [user setting]"
    exit 1
fi

if [ -f ./boards/$1 ]; then
    echo "Wrong board name"
    exit 1
fi

if [ -f ./user/$2 ]; then
    echo "Wrong user setting name"
    exit 1
fi

cp ./boards/$1/board.mk .
cp ./user/$2/user.mk .
