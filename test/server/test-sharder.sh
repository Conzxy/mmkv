#!/bin/bash
cd ../../build/test

timeout -k 2s 10s ./trackerd_test > x &
timeout -k 2s 10s ./shardd_test 10000 shard1 | grep start &
timeout -k 2s 10s ./shardd_test 10001 shard2 | grep start &

exit 0
