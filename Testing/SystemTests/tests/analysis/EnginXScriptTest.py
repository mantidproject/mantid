# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)
import systemtesting
import mantid.simpleapi as simple
from Engineering.EnginX import EnginX


class CreateVanadiumTest(systemtesting.MantidSystemTest):

    def runTest(self):
        test = EnginX(user="test", vanadium_run="236516",
                      directory="/home/sjenkins/Work/Build-1/ExternalData/Testing/Data/SystemTest")
        test.create_vanadium()

    def validate(self):
        return "eng_vanadium_integration", "engggui_vanadium_integration.nxs"

    def cleanup(self):
        simple.mtd.clear()


class createCalibrationWholeTest(systemtesting.MantidSystemTest):

    def runTest(self):
        test = EnginX(user="test", vanadium_run="236516",
                      directory="/home/sjenkins/Work/Build-1/ExternalData/Testing/Data/SystemTest")
        test.create_calibration()


