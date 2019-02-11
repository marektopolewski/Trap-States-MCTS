cmake_minimum_required(VERSION 3.13)
project(mctsf_ecplise)

set(CMAKE_CXX_STANDARD 14)
set(-DCMAKE_SH="CMAKE_SH-NOTFOUND")

include_directories(.)

add_executable(mctsf_ecplise
        benchmark.cpp
        bitbase.cpp
        bitboard.cpp
        bitboard.h
        bitcount.h
        book.cpp
        book.h
        endgame.cpp
        endgame.h
        evaluate.cpp
        evaluate.h
        history.h
        lock.h
        main.cpp
        material.cpp
        material.h
        misc.cpp
        misc.h
        montecarlotreenode.cpp
        montecarlotreenode.h
        move.cpp
        move.h
        movegen.cpp
        movegen.h
        movepick.cpp
        movepick.h
        pawns.cpp
        pawns.h
        position.cpp
        position.h
        psqtab.h
        rkiss.h
        search.cpp
        search.h
        similairty.cpp
        similarity.h
        thread.cpp
        thread.h
        timeman.cpp
        timeman.h
        tt.cpp
        tt.h
        types.h
        uci.cpp
        ucioption.cpp
        ucioption.h
        uctsearch.cpp
        uctsearch.h)
