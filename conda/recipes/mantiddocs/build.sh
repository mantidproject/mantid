#!/usr/bin/env bash
set -ex

# QtHelp docs build needs to start a QApplication so we need an X server on Linux
XVFB_SERVER_NUM=101

function run_with_xvfb {
    if [[ $OSTYPE == "linux"* ]]; then
        # Use -e because a bug on RHEL7 means --error-file produces an error:
	#   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=337703;msg=2
        # Use -noreset because of an X Display bug caused by a race condition in xvfb:
	#  https://gitlab.freedesktop.org/xorg/xserver/-/issues/1102
        xvfb-run -e /dev/stderr --server-args="-core -noreset -screen 0 640x480x24" \
        --server-num=${XVFB_SERVER_NUM} $@
    else
        eval $@
    fi
}

function terminate_xvfb_sessions {
    if [[ $OSTYPE == "linux"* ]]; then
        echo "Terminating any existing Xvfb sessions"

        # Kill Xvfb processes
        killall Xvfb || true

        # Remove Xvfb X server lock files
        rm -f /tmp/.X${XVFB_SERVER_NUM}-lock || true
    fi
}

mkdir build
cd build

cmake \
  ${CMAKE_ARGS} \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=$PREFIX \
  -DCMAKE_FIND_FRAMEWORK=LAST \
  -DMANTID_FRAMEWORK_LIB=SYSTEM \
  -DMANTID_QT_LIB=SYSTEM \
  -DENABLE_WORKBENCH=OFF \
  -DENABLE_DOCS=ON \
  -DDOCS_DOTDIAGRAMS=ON \
  -DDOCS_SCREENSHOTS=ON \
  -DDOCS_MATH_EXT=sphinx.ext.imgmath \
  -DDOCS_PLOTDIRECTIVE=ON \
  -DPACKAGE_DOCS=ON \
  -DENABLE_PRECOMMIT=OFF \
  -DCONDA_BUILD=True \
  -DCONDA_ENV=True \
  -DUSE_PYTHON_DYNAMIC_LIB=OFF \
  -DQt5_DIR=$BUILD_PREFIX/lib/cmake/qt5 \
  -DCPACK_PACKAGE_SUFFIX="" \
  -GNinja \
  ../

cmake --build .
run_with_xvfb cmake --build . --target docs-qthelp
terminate_xvfb_sessions
cmake --build . --target install
