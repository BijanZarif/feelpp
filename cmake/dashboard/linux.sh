#!/bin/bash
#set -x
# $1 provides the path to the toplevel source Feel++ directory
# e.g. $HOME/Devel/FEEL/feelpp.git
COMMON="ctest -VV -S $1/cmake/dashboard/testsuite.cmake,FEELPP_CTEST_CONFIG=$1/cmake/dashboard/feelpp.site.`hostname -s`.cmake,FEELPP_MODE=$2"
#$COMMON-gcc-4.4.6,FEELPP_CXX=g++-4.4,FEELPP_EXPLICIT_VECTORIZATION=novec
#$COMMON-gcc-4.4.6,FEELPP_CXX=g++-4.4,FEELPP_EXPLICIT_VECTORIZATION=SSE2
#$COMMON-gcc-4.5.2,FEELPP_CXX=g++-4.5,FEELPP_EXPLICIT_VECTORIZATION=novec
if [ -x /usr/bin/g++-4.5 ]; then
    $COMMON,FEELPP_CXXNAME=gcc-4.5.3,FEELPP_CXX=/usr/bin/g++-4.5,FEELPP_EXPLICIT_VECTORIZATION=SSE2
fi
if [ -x /usr/bin/g++-4.6 ]; then
    $COMMON,FEELPP_CXXNAME=gcc-4.6.2,FEELPP_CXX=/usr/bin/g++-4.6,FEELPP_EXPLICIT_VECTORIZATION=SSE2
fi
if [ -x /usr/bin/g++-4.7 ]; then
    $COMMON,FEELPP_CXXNAME=gcc-4.7.0,FEELPP_CXX=/usr/bin/g++-4.7,FEELPP_EXPLICIT_VECTORIZATION=SSE2
fi
if [ -x /usr/bin/clang++ ]; then
    $COMMON,FEELPP_CXXNAME=clang-3.1,FEELPP_CXX=/usr/bin/clang++,FEELPP_EXPLICIT_VECTORIZATION=SSE2
fi
