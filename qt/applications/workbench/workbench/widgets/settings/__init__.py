# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This is the main settings package. It's goal is to provide the main window
and contain the sub-sections of the settings, as well as run any saving operations
when "Save Settings" is clicked.

Each sub-section of the settings should be in a new package inside the `settings` package.
This allows separation of the sections, into separate MVPs with their own unit testing.

A current example of this is the `settings.general` package, which provides the General
section within the settings. Any triggers within that section are handled
in the `settings.general.presenter` package.
"""
