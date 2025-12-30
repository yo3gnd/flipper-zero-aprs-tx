#!/bin/sh
set -e

mkdir -p tests/build
cc -std=gnu99 -Wall -Wextra tests/packet_test.c src/packet.c src/aprs.c -o tests/packet_test
./tests/packet_test
cc -std=gnu99 -Wall -Wextra tests/aprs_test.c src/aprs.c src/packet.c -o tests/aprs_test
./tests/aprs_test
cc -std=gnu99 -Wall -Wextra -Itests/unity tests/aprs_test_new.c tests/unity/unity.c src/aprs.c src/packet.c -o tests/build/aprs_test_new
./tests/build/aprs_test_new
