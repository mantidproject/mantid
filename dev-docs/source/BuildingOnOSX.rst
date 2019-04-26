.. _BuildingOnOSX:

================
Building on OS X
================

.. contents::
  :local:

The following are instructions to build on various versions of OS X:

##################################
OS X 10.9 using clang and macports
##################################

*Disclaimer*

This instruction considers that you either use macports or need them for your other development projects. It also 
considers that you need to compile Mantid by yourself. In other cases please either `download a Mantid dmg package <http://download.mantidproject.org/>`_ or follow the instructions below. Warning: 
it is not recommended to have both, homebrew and macports installed simultaneously on one mac.

Instruction
-----------
1. Install Xcode and macports following the instructions on https://guide.macports.org/chunked/installing.html if needed.

2. Install Mantid prerequisites via ``sudo port install package_name``

3. Things to take care about:

- By default, POCO libraries in macports are missing libCrypto and libNetSSL. If you have the POCO libraries already installed, uninstall them: ``sudo port uninstall poco``, then install as: ``sudo port install poco +ssl``.
- Install OpenCascade libraries as: ``sudo port install oce -tbb``.

- libNeXus: macports do not contain libNeXus.

  1. Download the source code from the `NeXus developers website <http://download.nexusformat.org/kits/>`_.
  2. Build and install it:
   
     .. code-block:: sh

       % ./configure --prefix=/opt/local
       % make
       % sudo make install

      You may need to install additional packages to be able to build libNeXus.
      
  3. libNeXus must be recompiled after update of the macports if it's dependencies have been updated. Otherwise it may depend on some non-existent libraries.
   
- jsoncpp: ``mantid/Code/Mantid/Framework/DataObjects/src/NoShape.cpp`` line 3 contains: ``#include <jsoncpp/json/json.h>`` but in macports there is no ``jsoncpp`` folder in the ``/opt/local/include``, ``json.h`` is located in ``/opt/local/include/json``. As a temporary solution, you may create a symbolic link:  

  .. code-block:: sh

    % sudo mkdir /opt/local/include/jsoncpp
    % cd /opt/local/include/jsoncpp
    % sudo ln -s ../json

4. Run cmake. It may be needed to specify the compiler as well as the path to include files. I use the following cmake options:

.. code-block:: sh

    cmake  -DCMAKE_C_COMPILER=/usr/bin/clang \  
      -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -G 'Unix Makefiles' \
      -DCMAKE_PREFIX_PATH=/opt/local \ 
      -DCMAKE_BUILD_TYPE=Release \ 
      -DENABLE_CPACK=True \
      -DPOCO_INCLUDE_DIR=/opt/local/include \
      -DQWTPLOT3D_INCLUDE_DIR=/opt/local/include/qwtplot3d \
      -DJSONCPP_INCLUDE_DIR=/opt/local/include \
      -DOPENCASCADE_INCLUDE_DIR=/opt/local/include/oce \
      -DPYTHON_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Headers \
      -DSIP_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Headers \
      -DPYTHON_NUMPY_INCLUDE_DIR=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/numpy/core/include \
      -DPYTHON_EXECUTABLE=/opt/local/bin/python \
      -DPYLINT_EXECUTABLE=/opt/local/bin/pylint-2.7 \
      -DSPHINX_EXECUTABLE=/opt/local/bin/sphinx-build-2.7 \
      -DPACKAGE_DOCS=FALSE \
      -DDOCS_HTML=TRUE \
      -DPYQT4_PATH=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PyQt4 \ 
      -DSITEPACKAGES_PATH=/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages \
      -DOPENSSL_ROOT_DIR=/opt/local \
      -DMAKE_VATES=FALSE \
      -DMACPORTS=TRUE \
      -DCMAKE_INSTALL_PREFIX=path_where_to_install_mantid  /path_to_repository/mantid/Code/Mantid

5. Pay attention that packaging of the documentation is switched off. I did not manage to build it for the moment.
6. Build mantid running ``make`` or ``make -j number_of_threads``
7. You may create the dmg package running the ``make package`` command
8. You may also install Mantid using the ``make install`` command. *Warning*: if you do not want to install Mantid in /Applications, correct the CMAKE_INSTALL_PREFIX in the ``cmake_install.cmake`` file in your build directory.

Building VATES
--------------
Starting from Mantid 3.4, it is possible to build it with VATES support using macports. 

1. Build Paraview using the following instruction: :ref:`BuildingVATES`.

2. Set cmake option ``-DMAKE_VATES=TRUE``

3. Set path to the paraview build directory: ``-DParaView_DIR=/put_your_path_here``

4. Run steps 6-7(8) to build/install Mantid


##########################################
OS X 10.10 and 10.11 using clang and Xcode
##########################################
These instructions are from the assumptions of a blank newly installed Mac and want to use the system python. Other python distributions may work but have not been tested. 

1. First install Xcode and then clone the mantid git repository.

- The last version to support OS X Mavericks is Xcode 6.2
- The last version to support OS X Yosemite is Xcode 7.2.1
- As of August 1, 2016, our OS X El Capitan build server is running Xcode 7.3.1

2. Install Apple's Command Line tools (without this then /usr/include will not exist)

.. code-block:: sh

         xcode-select --install

2. Install `Homebrew <http://brew.sh>`_. If you already have Homebrew and are upgrading the OS follow the `instructions here <http://ryantvenge.com/2014/09/ruby-homebrea-yosemite/>`_:

.. code-block:: sh

         ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

3. Add the necessary 'taps'. The last 4 are to use qt4.

In order to be able to 'tap' the ``mantidproject/mantid`` 'tap' we need to have a couple of packages installed

.. code-block:: sh

        brew install git
        brew install git-lfs

        brew tap homebrew/science
        brew tap mantidproject/mantid
        brew tap caskroom/cask
        brew tap cartr/qt4
        brew tap-pin cartr/qt4

5. Install required dependencies (will make a mantid-developer formula soon)
   If you plan on distributing your application bundle, change ``brew install`` to ``brew install --build-bottle``

.. code-block:: sh

        brew cask install xquartz
        # mactex is optional, needed for parsing equations in qthelp documentation.
        brew cask install mactex
        brew install openssl
        brew install cmake
        brew install qt@4 --with-qt3support --build-bottle
        # sip,pyqt and qscintilla2 bring in homebrew's python if
        # installed with --build-bottle. And add --build-from-source.
        brew install sip --build-from-source --without-python3
        brew install pyqt@4 --build-from-source --without-python3
        brew install qscintilla2qt4 --build-from-source
        brew install qt --build-bottle
        brew install pyqt --build-from-source
        brew install qscintilla2 --build-from-source --without-python3
        brew install poco --c++11
        brew install boost --c++11 
        # boost-python brings in homebrew's python if installed with --build-bottle.
        brew install boost-python --c++11 --build-from-source
        brew install gsl
        brew install hdf5 --c++11
        brew install libmxml
        brew install muparser
        #Several unit tests fail with NeXus v4.4.2
        #https://github.com/mantidproject/mantid/issues/17001
        brew install nexusformat --c++11
        brew install jsoncpp
        brew install tbb --c++11
        brew install opencascade --build-bottle
        brew install qwt5
        brew install qwtplot3d
        brew install google-perftools
        brew install librdkafka

6. Uninstall homebrew Python that some of the dependencies insist on installing

.. code-block:: sh

        brew uninstall python

6. Optional: for cmake-gui

.. code-block:: sh

        brew cask install cmake

7. Now to install the other python package dependencies:

.. code-block:: sh

        sudo easy_install pip
        sudo -H pip install sphinx
        # https://github.com/mantidproject/mantid/issues/13481
        sudo -H pip install "ipython[notebook]==3.2.1"
        # qtconsole only required with ipython 4+ 
        #sudo -H pip install qtconsole
        sudo -H pip install qtpy
        sudo -H pip install pygments
        sudo -H pip install pyzmq
        sudo -H pip install pycifrw
        # Version matches Windows/RHEL/Ubuntu (trusty)
        sudo -H pip install PyYAML==3.10
        # Version matches Windows/RHEL/Ubuntu (trusty)
        sudo -H pip install mock==1.0.1
        sudo -H pip install requests==2.9.1

8. Install the theme for sphinx

.. code-block:: sh

        sudo pip install sphinx_bootstrap_theme

9. Install other python dependencies


.. code-block:: sh

        sudo pip install psutil
        brew install h5py

9. Add Homebrew’s site-packages to your python path.

.. code-block:: sh

        mkdir -p ~/Library/Python/2.7/lib/python/site-packages
        echo '/usr/local/lib/python2.7/site-packages' > ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth

10. Now you need to patch a header in your python!

- If building on the command line with make or ninja.

  .. code-block:: sh

        cd /usr/include/python2.7

  or

  .. code-block:: sh

        cd /System/Library/Frameworks/Python.framework/Headers 

  then

  .. code-block:: sh

        sudo cp pyport.h pyport.h.original
        sudo patch pyport.h $MANTIDCHECKOUTROOT/buildconfig/pyport.patch

- If building with Xcode on OS X Yosemite 

  .. code-block:: sh
   
        cd /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.11.sdk/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7


  then

  .. code-block:: sh

        sudo cp pyport.h pyport.h.original
        sudo patch pyport.h $MANTIDCHECKOUTROOT/buildconfig/pyport.patch

  **Note**: If Xcode updates for any reason, the patch will be lost.


11. Now run CMake and select the Xcode generator with the default native compilers.

12. Now open the project in Xcode (doing this from the command line to ensure the PYTHONPATH is correctly picked up by Xcode).

    .. code-block:: sh

        cd /path/to/my/build/dir
        open Mantid.xcodeproj

Troubleshooting
---------------
1. The main problem that can arise is due to python path issues.  This usually either arises at the CMake or Run from Xcode steps.  It is because the PYTHONPATH is not being picked up.
2. If you have upgraded to Mavericks (OS X 10.9) from a previous version of OS X with homebrew already installed then you may encounter some issues related to the fact that the default std lib has changed.  The easiest way to avoid this is to remove and then re-install all your formulas.
3. You may find that if you build the ``MantidPlot`` target then you will get errors when you run, such as *Can't start python* and *Cannot load Curve Fitting Plugins*, this is due to the fact that the MantidPlot target does not contain all the dependencies.  You are best, if you are unsure of the hierarchy, to just use the ALL_BUILD target and then just switch to the MantidPlot target in order to run.
4. NOTE that you might need to run ``./MantidPlot.app/Contents/MacOS/MantidPlot`` from the ``BUILD-DIR/bin`` (instead of ``open MantidPlot.app`` OR ``./MantidPlot`` from ``BUILD-DIR/bin/MantidPlot.app/Contents/MacOS/``) to get the library paths correct. Otherwise the issues above might show up (at least on OS X 10.11 El Capitan).
5. Upgrading HDF5 requires also rebuilding nexusformat, h5py, and ParaView.  


##########
OS X 10.12
##########
The following instructions setup the build environment for mantid using clang compiler and python provided by the system, and all the other dependencies installed with brew. The drawback is that one has little control over python version and OpenMP will not be found. Make sure you have Qt Creator IDE and optionally cmake (GUI) app installed.

1. Install Xcode from AppStore
2. Install Xcode command line tools 

.. code-block:: sh

    xcode-select --install

3. Install home-brew package manager

.. code-block:: sh

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

4. Add the necessary 'taps'

In order to be able to 'tap' the `mantidproject/mantid` 'tap' we need to have a couple of packages installed

.. code-block:: sh

    brew install git
    brew install git-lfs

    brew tap mantidproject/mantid
    brew tap caskroom/cask
    brew tap cartr/qt4
    brew tap-pin cartr/qt4

5. Install the necessary dependencies. Note that some of these will bring brew python with them as dependency.

.. code-block:: sh

    brew cask install xquartz
    brew cask install mactex
    brew install openssl
    brew install cmake
    brew install ninja --without-test
    brew install qt@4 --build-bottle
    brew install sip --build-from-source --without-python
    brew install pyqt@4 --build-from-source
    brew install qscintilla2qt4 --build-from-source --without-python
    brew install qt --build-bottle
    brew install pyqt --build-from-source
    brew install qscintilla2 --build-from-source --without-python
    brew install poco
    brew install boost --c++11
    brew install boost-python --c++11 --build-from-source
    brew install gsl
    brew install gcc
    brew install hdf5 --c++11
    brew install libmxml
    brew install muparser
    brew install nexusformat --c++11
    brew install jsoncpp
    brew install tbb --c++11
    brew install opencascade --build-bottle
    brew install qwt5
    brew install qwtplot3d
    brew install google-perftools
    brew install librdkafka

If, while configuring Mantid, cmake complains that it cannot find sip, uninstall the package by ``brew uninstall --ignore-dependencies sip``, reinstall it using the line above and follow the instructions on how to add Homebrew's site-packages to Python ``sys.path``.


6. Uninstall the brew python if it has been previously installed

.. code-block:: sh

    brew uninstall --ignore-dependencies python3

7. Install pip python package manager

.. code-block:: sh

    sudo easy_install pip

8. Install necessary python packages with pip

.. code-block:: sh

    sudo -H pip install sphinx --ignore-installed
    sudo -H pip install "ipython[notebook]==3.2.1"
    sudo -H pip install qtpy
    sudo -H pip install pycifrw
    sudo -H pip install PyYAML==3.10
    sudo -H pip install mock==1.0.1
    sudo pip install sphinx_bootstrap_theme
    sudo pip install psutil
    sudo pip install "matplotlib>=2.1.2"
    sudo pip install requests==2.9.1 

9. Install h5py

.. code-block:: sh

    brew install h5py

10. Add Homebrew’s site-packages to your python path.

.. code-block:: sh

    mkdir -p ~/Library/Python/2.7/lib/python/site-packages
    echo '/usr/local/lib/python2.7/site-packages' > ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth

11. Git clone the mantid repository

12. Disable the system integrity protection (SIP). To do this 

    - restart the computer
    - before the apple logo appears press `Command+R` to enter the recovery mode
    - when in recovery mode, go to `Utilities>Terminal` and type

      .. code-block:: sh

        csrutil disable
   
    - reboot again

13. Now that SIP is disabled we can do the necessary patch:

.. code-block:: sh

    cd /usr/include/python2.7
    sudo cp pyport.h pyport.h.original
    sudo patch pyport.h $MANTIDCHECKOUTROOT/buildconfig/pyport.patch

14. Enable again the system integrity protection by repeating Step 12 and typing this time:

.. code-block:: sh

    csrutil enable

15. Open mantid project from Qt Creator, and you should be able to run cmake and build, given the right environment:

.. code-block:: sh

    CC=/usr/bin/clang
    CXX=/usr/bin/clang++
    PATH=/usr/local/bin/:$PATH

Local bin contains the symlink to the brew packages, which have to come first in path, before `/usr/bin`. That's why it is important not to have python or clang (with this setup) in brew.
    

16. Add to your `.profile`

.. code-block:: sh

    export PYTHONPATH=$BUILDMANTID/bin


17. You should now be able to mantid.
