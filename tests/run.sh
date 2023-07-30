#!/bin/sh
set -e
cc -std=c99 -Wall -Wextra -pedantic tests/packet_test.c packet.c aprs.c -o tests/packet_test
./tests/packet_test
cc -std=c99 -Wall -Wextra -pedantic tests/aprs_test.c aprs.c -o tests/aprs_test
./tests/aprs_test
