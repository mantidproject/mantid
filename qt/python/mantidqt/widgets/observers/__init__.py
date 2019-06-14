# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

"""
This module provides common functionality for GUI classes observers changes in the ADS.

Inheriting the classes from this module provides a few helper methods to prevent
leaking the ADS Observer object, and making sure that the view is deleted after closing.
"""
