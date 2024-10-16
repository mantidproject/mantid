# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-few-public-methods
"""
These system tests are to verify that the IDF and parameter files for POLREF, CRISP, INTER and SURF are read properly
"""

import systemtesting
from mantid.kernel import config
from mantid.simpleapi import LoadEmptyInstrument
import os
from abc import ABCMeta, abstractmethod


class ISISReflInstrumentIDFTest(systemtesting.MantidSystemTest, metaclass=ABCMeta):
    @abstractmethod
    def get_IDF_name(self):
        """Returns the IDF"""
        raise NotImplementedError("Implement get_IDF_name to return ")

    def runTest(self):
        IDF_path = os.path.join(config["instrumentDefinition.directory"], self.get_IDF_name())
        ws = LoadEmptyInstrument(IDF_path)
        inst = ws.getInstrument()
        self.assertTrue(isinstance(inst.getNumberParameter("MonitorIntegralMin")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("MonitorIntegralMax")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("MonitorBackgroundMin")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("MonitorBackgroundMax")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("PointDetectorStart")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("PointDetectorStop")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("MultiDetectorStart")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("I0MonitorIndex")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("LambdaMin")[0], float))
        self.assertTrue(isinstance(inst.getNumberParameter("LambdaMax")[0], float))

        return True

    def doValidate(self):
        return True


# Specialisation for testing POLREF


class POLREF_ISISReflInstrumentIDFTest(ISISReflInstrumentIDFTest):
    def get_IDF_name(self):
        return "POLREF_Definition.xml"


# Specialisation for testing INTER


class INTER_ISISReflInstrumentIDFTest(ISISReflInstrumentIDFTest):
    def get_IDF_name(self):
        return "INTER_Definition.xml"


# Specialisation for testing SURF


class SURF_ISISReflInstrumentIDFTest(ISISReflInstrumentIDFTest):
    def get_IDF_name(self):
        return "SURF_Definition.xml"


# Specialisation for testing CRISP


class CRISP_ISISReflInstrumentIDFTest(ISISReflInstrumentIDFTest):
    def get_IDF_name(self):
        return "CRISP_Definition.xml"
