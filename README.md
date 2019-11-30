# Trap-Adaptive Monte Carlo Tree Search Chess Agent

My Bachelor's thesis project researched under the supervision of Paolo Turrini for my Bachelor's thesis. The dissertation consists of a final report and an implementation of a C++ Chess engine utilising developed algorithm.

The subject of thesis revolves around the Game Theory and agents based on MCTS. Despite algorihtms exceptional performance in games like Go, the problem of expanding trap states outweights benefits of MCTS in domains such as Chess. The aim of the project is to address this challenge to allow further advancements in the MCTS emerging as an optimal algorithm capable of General Game Playing.

# Repo
|Item|Type|Description|
|-|-|-|
|`source/`    | dir  | source code for the Chess agent based on Stockfish |
|`test/`      | dir  | input files for correctness and performance testing |
|`report.pdf` | file | final report for the project (created in LaTeX) |

# Testing

<b>Disclaimer!</b>\
To test the software a Windows machine is required due to platform dependency in the `misc.cpp` controller. For other systems manual adjustments to the file are required.

1. Clone directory: \
```git clone https://github.com/marektopolewski/Trap-States-MCTS.git```

2. Create a new subdirectory and open it: \
```mkdir build && cd build```

3. Run cmake:\
`cmake ..`

4. Create the binary:\
`make`