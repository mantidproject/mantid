# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI, NScD Oak Ridge
# National Laboratory, European Spallation Source, Institut Laue - Langevin &
# CSNS, Institute of High Energy Physics, CAS SPDX - License - Identifier: GPL -
# 3.0 +

#[=======================================================================[.rst:
FindPyQt5
---------

Find the installed version of the PyQt5 libraries.
FindPyQt5 should be called after Python has been found.

PyQt5 website: https://www.riverbankcomputing.com/static/Docs/PyQt5/

Result Variables
^^^^^^^^^^^^^^^^

This module define the following variables:

``PYQT5_FOUND``
  True if PyQt5 has been found.

``PYQT5_VERSION``
  The version of PyQt5 found expressed as a 6 digit hex number
  suitable for comparison as a string

``PYQT5_VERSION_STR``
  The version of PyQt5 as a human readable string.

``PYQT5_VERSION_TAG``
  The Qt5 version tag used by PyQt's sip files.

``PYQT5_SIP_DIR``
  The directory holding the PyQt5 .sip files.

``PYQT5_SIP_FLAGS``
  The SIP flags used to build PyQt5.

``PYQT5_SIP_ABI_VERSION``
  The version of the sip ABI used to build against in the >=v6 build system

#]=======================================================================]
# Implementation is defined separately to easily share between multiple PyQt
# versions
include(PyQtFindImpl)

find_pyqt(5)
