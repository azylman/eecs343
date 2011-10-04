#!/bin/sh

TEST="$1"

echo "Actual output:\n-----------------------"
testsuite/sdriver.pl -t testsuite/test$TEST.in -s ./tsh

echo "Predictied output:\n-----------------------"
testsuite/sdriver.pl -t testsuite/test$TEST.in -s testsuite/tsh-ref

