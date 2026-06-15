# PractRandの所在を仮定。
cd /home/user/dev/PractRand && g++ -std=c++14 -O3 -Iinclude src/*.cpp src/RNGs/*.cpp src/RNGs/other/*.cpp tools/RNG_test.cpp -o RNG_test -lpthread 2>&1