git clone --recurse-submodules https://github.com/bkaradzic/bgfx.cmake.git 
cd bgfx.cmake
mkdir bld
cd bld
cmake .. -DENABLE_ARC=FALSE -DBGFX_BUILD_EXAMPLES=off -DBGFX_CUSTOM_TARGETS=off -DCMAKE_BUILD_TYPE=Release -DBGFX_BUILD_TOOLS=off -DCMAKE_TOOLCHAIN_FILE=../../sqwar/ios.toolchain.cmake -DPLATFORM=OS64
make -j8
make install