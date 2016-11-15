#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *
import numpy as np


class VisionLoadDetectorTable(PythonAlgorithm):

    def category(self):
        return "Utility\\Development"

    def summary(self):
        return "Warning - This is under development - Algorithm to load detector parameters for VISION."

    def PyInit(self):
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output),
                             doc="Name of Output Workspace")

        self.declareProperty(FileProperty("DetectorFile", "", action=FileAction.Load, extensions=['csv']),
                             doc="Name of detector file to load.")

    def PyExec(self):
        filename = self.getPropertyValue("DetectorFile")
        output_ws_name = self.getPropertyValue("OutputWorkspace")

        # Open File and read parameters
        spectra,l1,l2,twotheta,efixed,emode = np.genfromtxt(filename, delimiter=',', unpack=True)

        # Setup the output table
        output_workspace = CreateEmptyTableWorkspace(OutputWorkspace=output_ws_name)
        output_workspace.addColumn("int", "spectra")
        output_workspace.addColumn("double", "l1")
        output_workspace.addColumn("double", "l2")
        output_workspace.addColumn("double", "twotheta")
        output_workspace.addColumn("double", "efixed")
        output_workspace.addColumn("int", "emode")

        # Write the values
        for i in range(len(spectra)):
            output_workspace.addRow([int(spectra[i]),float(l1[i]),float(l2[i]),
                                     float(twotheta[i]),float(efixed[i]),int(emode[i])])

        # Set the output workspace
        self.setProperty("OutputWorkspace", output_workspace)

AlgorithmFactory.subscribe(VisionLoadDetectorTable)
