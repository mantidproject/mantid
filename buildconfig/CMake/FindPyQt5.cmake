# Find PyQt5
# ~~~~~~~~~~
#
# PyQt5 website: http://www.riverbankcomputing.co.uk/pyqt/index.php
#
# Find the installed version of PyQt5. FindPyQt5 should only be called after
# Python has been found.
#
# This file defines the following variables, which can also be overridden by
# users:
#
# PYQT5_VERSION - The version of PyQt5 found expressed as a 6 digit hex number
#     suitable for comparison as a string
#
# PYQT5_VERSION_STR - The version of PyQt5 as a human readable string.
#
# PYQT5_VERSION_TAG - The Qt5 version tag used by PyQt's sip files.
#
# PYQT5_SIP_DIR - The directory holding the PyQt5 .sip files.
#
# PYQT5_SIP_FLAGS - The SIP flags used to build PyQt.

include (PyQtFindImpl)
find_pyqt (5)
