#!/bin/sh

TESTFILE=$(mktemp /tmp/netsendXXXXXX)
NETSEND_BIN=./netsend
TEST_FAILED=0

pre()
{
  # generate a file for transfer
  echo Initialize test environment
  dd if=/dev/zero of=${TESTFILE} bs=4k count=10000 1>/dev/null 2>&1
}

post()
{
  echo Cleanup test environment
  rm -f ${TESTFILE}
}

die()
{
  post
  exit 1
}

further_help()
{
  echo "\nOne or more testcases failed somewhere!"
  echo "A series of causales are possible:"
  echo "  o No kernel support for a specific protocol (e.g. tipc, ipv6, ...)"
  echo "  o Architecture bugs (uncatched kernel, glibc or netsend bugs)"
  echo "  o Last but not least: you triggered a real[tm] netsend error ;)"
  echo "\nIf the testcases doesn't affect you you can skip the test"
  echo "But if you want to dig into this corner you should start this script"
  echo "via the debug mode (\"sh -x unit_test.sh\"), get out the executed netsend command"
  echo "and execute them manually on the commandline and understand the failure message!"
  echo "\nFor further information or help -> http://netsend.berlios.de"

}

case1()
{
  echo -n "Trivial receive transmit test ..."

  ${NETSEND_BIN} tcp receive &
  RPID=$!

  # give netsend 2 seconds to set up everything and
  # bind properly
  sleep 2

  ${NETSEND_BIN} tcp transmit ${TESTFILE} localhost &

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi

}

case2()
{
  echo -n "IPv4 (AF_INET) enforce test ..."

  ${NETSEND_BIN} -4 tcp receive 1>/dev/null 2>&1 &
  RPID=$!

  # give netsend 2 seconds to set up everything and
  # bind properly
  sleep 2

  ${NETSEND_BIN} -4 tcp transmit ${TESTFILE} localhost 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    echo testcase failed
    die
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi

}

case3()
{
  echo -n "IPv6 (AF_INET6) enforce test ..."

  ${NETSEND_BIN} -6 tcp receive 1>/dev/null 2>&1 &
  RPID=$!

  # give netsend 2 seconds to set up everything and
  # bind properly
  sleep 2

  ${NETSEND_BIN} -6 tcp transmit ${TESTFILE} localhost 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    TEST_FAILED=1
  else
    passed
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else 
    echo passed
  fi

}

case4()
{
  echo -n "Statictic output test (both, machine and human) ..."

  L_ERR=0

  # start with human output

  R_OPT="-T human tcp receive"
  T_OPT="-T human tcp transmit ${TESTFILE} localhost"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  # give netsend 2 seconds to set up everything and
  # bind properly
  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # and machine output

  R_OPT="-T machine tcp receive"
  T_OPT="-T machine tcp transmit ${TESTFILE} localhost"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  # give netsend 2 seconds to set up everything and
  # bind properly
  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  if [ $L_ERR -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi
}

case5()
{
  echo -n "TCP protocol tests ..."

  L_ERR=0

  R_OPT="tcp receive"
  T_OPT="tcp transmit ${TESTFILE} localhost"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  if [ $L_ERR -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi
}

case6()
{
  echo -n "DCCP protocol tests ..."

  L_ERR=0

  R_OPT="dccp receive"
  T_OPT="dccp transmit ${TESTFILE} localhost"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  if [ $L_ERR -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi
}

case7()
{
  echo -n "SCTP protocol tests ..."

  L_ERR=0

  R_OPT="dccp receive"
  T_OPT="dccp transmit ${TESTFILE} localhost"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  if [ $L_ERR -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi
}

case8()
{
  echo -n "TIPC protocol tests ..."
  
  L_ERR=0

  R_OPT="tipc receive -t SOCK_STREAM"
  T_OPT="-u rw tipc transmit -t SOCK_STREAM ${TESTFILE}"

  ${NETSEND_BIN} ${R_OPT} 1>/dev/null 2>&1 &
  RPID=$!

  sleep 2

  ${NETSEND_BIN} ${T_OPT} 1>/dev/null 2>&1
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  # wait for receiver and check return code
  wait $RPID
  if [ $? -ne 0 ] ; then
    L_ERR=1
  fi

  if [ $L_ERR -ne 0 ] ; then
    echo failed
    TEST_FAILED=1
  else
    echo passed
  fi
}

echo "\nnetsend unit test script - (C) 2007\n"

pre

trap post INT

case1
case2
case3
case4
case5
case6
case7
case8

post

if [ $TEST_FAILED -ne 0 ] ; then
  further_help
fi


