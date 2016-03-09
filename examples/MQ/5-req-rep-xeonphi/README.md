Example 5: Request & Reply with Xeon Phi
===============

Compile FairRoot with intel compiler
cmake -DCMAKE_CXX_COMPILER=icpc -DCMAKE_C_COMPILER=icc -DUSE_PATH_INFO=true -DUSE_DIFFERENT_COMPILER=TRUE  ../FairRoot/

cmake -DCMAKE_CXX_COMPILER=icpc -DCMAKE_C_COMPILER=icc -DUSE_PATH_INFO=true -DCMAKE_CXX_FLAGS=mic -DUSE_DIFFERENT_COMPILER=TRUE  ../FairRoot/


cmake -DCMAKE_CXX_COMPILER=icpc -DCMAKE_C_COMPILER=icc -DUSE_PATH_INFO=true -DUSE_DIFFERENT_COMPILER=TRUE -DCMAKE_INSTALL_PREFIX=/home/andrey/Development/fairroot/install -DUSE_DIFFERENT_COMPILER=TRUE -DCMAKE_CXX_FLAGS="-offload=optional" ../FairRoot/
