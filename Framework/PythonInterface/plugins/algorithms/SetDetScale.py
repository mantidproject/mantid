from __future__ import (absolute_import, division, print_function)

import math
import numpy as np
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode, \
    ITableWorkspaceProperty, FileAction, FileProperty, WorkspaceProperty, InstrumentValidator, Progress
from mantid.kernel import Direction, FloatBoundedValidator, PropertyCriterion, EnabledWhenProperty, \
    logger, Quat, V3D, StringArrayProperty, StringListValidator
import mantid.simpleapi as api

class SetDetScale(PythonAlgorithm):
    """
    Class to set instrument detScale for SaveHKL and AnvredCorrection
    """

    def category(self):
        """
        Mantid required
        """
        return "DataHandling\\Instrument"

    def name(self):
        """
        Mantid required
        """
        return "SetDetScale"

    def summary(self):
        """
        Mantid required
        """
        return "Set/change the detScale parameters for a MatrixWorkspace or PeaksWorkspace instrument"

    def PyInit(self):

        self.declareProperty(WorkspaceProperty("Workspace", "",
                                               validator=InstrumentValidator(),
                                               optional=PropertyMode.Optional,
                                               direction=Direction.InOut),
                             doc="MatrixWorkspace or PeaksWorkspace with instrument.")

        # List of parameters
        self.declareProperty(StringArrayProperty("DetScaleList",
                                                 direction=Direction.Input),
                             doc="Comma separated list detectorNumbers:detScales eg. 13:1.046504,14:1.259293")



    def PyExec(self):
        WS = self.getProperty("Workspace").value

        # Now input all the components 
        components = self.getProperty("DetScaleList").value

        listParse = []
        for component in components:
            comp, value = component.split(":")
            listParse.append({"ParameterName":"detScale"+comp, "Value":value})

        for d in listParse:
            api.SetInstrumentParameter(Workspace=WS,ParameterType="Number",**d)
        
# Register algorithm with Mantid.
AlgorithmFactory.subscribe(SetDetScale)
