#!/bin/sh
set -e
cc -std=gnu99 -Wall -Wextra tests/packet_test.c packet.c aprs.c -o tests/packet_test
./tests/packet_test
cc -std=gnu99 -Wall -Wextra tests/aprs_test.c aprs.c packet.c -o tests/aprs_test
./tests/aprs_test
