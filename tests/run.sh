#!/bin/sh
set -e
cc -std=c99 -Wall -Wextra -pedantic tests/packet_test.c packet.c -o tests/packet_test
./tests/packet_test
