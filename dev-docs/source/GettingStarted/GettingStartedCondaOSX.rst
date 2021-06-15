.. _GettingStartedCondaOSX:

==============
Conda on MacOS
==============

- `brew install miniconda`
    - Make sure to init conda
- `conda env create -f {SOURCE}/mantid-development-osx.yml`
- `conda activate mantid`
MacOS Build:
- cd {SOURCE}
- mkdir build
- cd build
- `cmake {SOURCE} --preset=osx`
    - Alternatively if you don't want to have your build folder in your source then pass these arguments to cmake:
        `cmake {SOURCE} -GNinja -DCMAKE_FIND_FRAMEWORK=LAST -DCMAKE_PREFIX_PATH=${CONDA_PREFIX} -DUSE_PYTHON_DYNAMIC_LIB=OFF -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/qt5 -DHDF5_ROOT=${CONDA_PREFIX} -DOPENSSL_ROOT=${CONDA_PREFIX}`
- ninja
