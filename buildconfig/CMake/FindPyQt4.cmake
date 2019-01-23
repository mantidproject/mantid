# Find PyQt4
# ~~~~~~~~~~
#
# PyQt4 website: http://www.riverbankcomputing.co.uk/pyqt/index.php
#
# Find the installed version of PyQt4. FindPyQt4 should only be called after
# Python has been found.
#
# This file defines the following variables, which can also be overridden by
# users:
#
# PYQT4_VERSION - The version of PyQt4 found expressed as a 6 digit hex number
#     suitable for comparison as a string
#
# PYQT4_VERSION_STR - The version of PyQt4 as a human readable string.
#
# PYQT4_VERSION_TAG - The Qt4 version tag used by PyQt's sip files.
#
# PYQT4_SIP_DIR - The directory holding the PyQt4 .sip files.
#
# PYQT4_SIP_FLAGS - The SIP flags used to build PyQt.

include (PyQtFindImpl)
find_pyqt (4)
