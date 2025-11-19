rm -rf build
mkdir "build"
cd build

cmake -DLINUX_BUILD=ON ..
    
make -j8