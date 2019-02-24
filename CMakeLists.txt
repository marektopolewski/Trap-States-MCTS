cmake_minimum_required(VERSION 3.13)
project(mctsf_ecplise)

set(CMAKE_CXX_STANDARD 14)
set(-DCMAKE_SH="CMAKE_SH-NOTFOUND")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(.)

add_executable(mctsf_ecplise
        source/benchmark.cpp
        source/bitbase.cpp
        source/bitboard.cpp
        source/bitboard.h
        source/bitcount.h
        source/book.cpp
        source/book.h
        source/endgame.cpp
        source/endgame.h
        source/evaluate.cpp
        source/evaluate.h
        source/history.h
        source/lock.h
        source/main.cpp
        source/material.cpp
        source/material.h
        source/misc.cpp
        source/misc.h
        source/montecarlotreenode.cpp
        source/montecarlotreenode.h
        source/move.cpp
        source/move.h
        source/movegen.cpp
        source/movegen.h
        source/movepick.cpp
        source/movepick.h
        source/pawns.cpp
        source/pawns.h
        source/position.cpp
        source/position.h
        source/psqtab.h
        source/rkiss.h
        source/search.cpp
        source/search.h
        source/similairty.cpp
        source/similarity.h
        source/thread.cpp
        source/thread.h
        source/timeman.cpp
        source/timeman.h
        source/tt.cpp
        source/tt.h
        source/types.h
        source/uci.cpp
        source/ucioption.cpp
        source/ucioption.h
        source/uctsearch.cpp
        source/uctsearch.h)
