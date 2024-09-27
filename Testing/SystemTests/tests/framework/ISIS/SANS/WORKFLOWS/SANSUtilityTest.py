# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-few-public-methods

import systemtesting
from ISIS.SANS.isis_sans_system_test import ISISSansSystemTest
from mantid.simpleapi import *
import SANSUtility as su
import SANSadd2 as add

import os

from sans_core.common.enums import SANSInstrument


def unixLikePathFromWorkspace(ws):
    return su.getFilePathFromWorkspace(ws).replace("\\", "/")


@ISISSansSystemTest(SANSInstrument.LOQ)
class SANSUtilityTest(systemtesting.MantidSystemTest):
    def runTest(self):
        # created after issue reported in #8156
        ws = Load("LOQ54432")
        self.assertTrue("LOQ/LOQ54432.raw" in unixLikePathFromWorkspace(ws))
        ws = Load("LOQ99618.RAW")
        self.assertTrue("LOQ/LOQ99618.RAW" in unixLikePathFromWorkspace(ws))
        add.add_runs(("LOQ54432", "LOQ54432"), "LOQ", ".raw")
        ws = Load("LOQ54432-add")
        file_path = unixLikePathFromWorkspace(ws)
        logger.information("File Path from -add: " + str(file_path))
        file_path = file_path.replace("-ADD", "-add")  # MAC seems to report that the file is LOQ54432-ADD.nxs
        self.assertTrue("LOQ54432-add" in file_path)
        os.remove(file_path)
