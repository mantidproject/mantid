include(ExternalProject)
include(ProcessorCount)

set(_SIP_PYQT_DIR extern-pyt4-sip)
set(_SIP_PYQT_INSTALL_DIR ${_SIP_PYQT_DIR}/install)

# sipdir - use different variables to standard sip installation to be able to distingish it
set(PYQT4_SIP_INCLUDE_DIR "${CMAKE_BINARY_DIR}/${_SIP_PYQT_INSTALL_DIR}/include"  CACHE STRING "sip include directory" FORCE)
set(PYQT4_SIP_EXECUTABLE "${CMAKE_BINARY_DIR}/${_SIP_PYQT_INSTALL_DIR}/bin/sip" CACHE STRING "sip executable" FORCE)
set(PYQT4_SIP_VERSION "041307" CACHE STRING "sip hexadecimal string" FORCE)
set(PYQT4_SIP_VERSION_STR "4.19.7" CACHE STRING "sip version string" FORCE)
ExternalProject_Add(extern-pyqt4-sip
  PREFIX ${_SIP_PYQT_DIR}/sip
  INSTALL_DIR ${_SIP_PYQT_INSTALL_DIR}
  URL https://www.riverbankcomputing.com/static/Downloads/sip/4.19.7/sip-4.19.7.tar.gz
  URL_HASH MD5=ae4f2db79713046d61b2a44e5ee1e3ab
  CONFIGURE_COMMAND "${PYTHON_EXECUTABLE}" "<SOURCE_DIR>/configure.py"
    --sip-module=PyQt4.sip
    --bindir=<INSTALL_DIR>/bin
    --destdir=<INSTALL_DIR>/lib/site-packages
    --incdir=<INSTALL_DIR>/include
    --sipdir=<INSTALL_DIR>/share/sip
  BUILD_COMMAND make 2> build.log
)

# PyQt4
set(PYQT4_VERSION "040c01" CACHE STRING "PyQt4's version as a 6-digit hexadecimal number" FORCE)
set(PYQT4_VERSION_STR "4.12.1" CACHE STRING "PyQt4's version as a human-readable string" FORCE)
set(PYQT4_VERSION_TAG "Qt_4_8_6" CACHE STRING "The Qt version tag used by PyQt4's .sip files" FORCE)
set(PYQT4_SIP_DIR "${CMAKE_BINARY_DIR}/${_SIP_PYQT_INSTALL_DIR}/share/sip" CACHE PATH "The base directory where PyQt4's .sip files are installed" FORCE)
set(PYQT4_SIP_FLAGS "-x VendorID -t WS_X11 -x PyQt_NoPrintRangeBug -t Qt_4_8_6" CACHE STRING "The SIP flags used to build PyQt4" FORCE)
set(PRIVATE_PYQT_SITE_PACKAGES ${CMAKE_BINARY_DIR}/${_SIP_PYQT_INSTALL_DIR}/lib/site-packages)
set(_pyqt4_lib_site_packages ${PRIVATE_PYQT_SITE_PACKAGES}/PyQt4)

# Write a wrapper pyuic script so it can find out internal copy of PyQt4
set(PYQT4_PYUIC "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/internal-pyuic.py" CACHE STRING "Location of the pyuic script" FORCE)
configure_file(${CMAKE_MODULE_PATH}/internal-pyuic.py.in ${PYQT4_PYUIC} @ONLY)

# Determine core count for make step
ProcessorCount(NPROCESSORS)
if(NPROCESSORS EQUAL 0)
  set(NPROCESSORS 1)
endif()

ExternalProject_Add(extern-pyqt4
  PREFIX ${_SIP_PYQT_DIR}/pyqt4
  INSTALL_DIR ${_SIP_PYQT_INSTALL_DIR}
  URL https://sourceforge.net/projects/pyqt/files/PyQt4/PyQt-4.12.1/PyQt4_gpl_x11-4.12.1.tar.gz/download
  URL_HASH MD5=0112e15858cd7d318a09e7366922f874
  PATCH_COMMAND patch -p1 --input ${CMAKE_SOURCE_DIR}/buildconfig/CMake/pyqt4_qreal_float_support.patch
    COMMAND patch -p0 --input ${CMAKE_SOURCE_DIR}/buildconfig/CMake/pyqt4_disable_unnecessary_modules.patch
    # patch configure to pick up sipconfig built above
    COMMAND sed -i -e "/^import sipconfig/i sys.path.insert(0, \"${_pyqt4_lib_site_packages}\")" "<SOURCE_DIR>/configure.py"
  CONFIGURE_COMMAND "${PYTHON_EXECUTABLE}" "<SOURCE_DIR>/configure.py"
    --assume-shared
    --confirm-license
    --bindir=<INSTALL_DIR>/bin
    --destdir=<INSTALL_DIR>/lib/site-packages
    --sipdir=<INSTALL_DIR>/share/sip
    --no-designer-plugin
    --no-timestamp
    --no-deprecated
    --qmake=/usr/bin/qmake-qt4
    --no-qsci-api
  BUILD_COMMAND make -j${NPROCESSORS} 2> build.log
  DEPENDS extern-pyqt4-sip
)

# Write out .pth file to find this. We ensure to insert our path ahead of others so it takes precendence over the system
# We assume we only have to support a single-configuration build type on Linux
file(WRITE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/private-pyqt4.pth
"import sys; sys.__plen = len(sys.path)
${PRIVATE_PYQT_SITE_PACKAGES}
${_pyqt4_lib_site_packages}
import sys; new = sys.path[sys.__plen:]; del sys.path[sys.__plen:]; p = getattr(sys, '__egginsert', 0); sys.path[p:p] = new; sys.__egginsert = p + len(new)
")

# Package PyQt. We assume this is for Python 3
install(DIRECTORY ${PRIVATE_PYQT_SITE_PACKAGES}/PyQt4
        DESTINATION ${LIB_DIR} 
        PATTERN "__pycache__" EXCLUDE
        PATTERN "port_v2" EXCLUDE)
