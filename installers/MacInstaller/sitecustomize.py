# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from pathlib import Path
import site
import sys

# Add extra site-packages containing our Python modules
_macos = Path(__file__).joinpath('../../../../../../../../MacOS')
site.addsitedir(_macos.resolve())
del _macos


