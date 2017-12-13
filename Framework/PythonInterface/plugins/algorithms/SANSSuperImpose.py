# pylint: disable=no-init

from __future__ import (absolute_import, division, print_function)

from mantid.api import (PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty,
                        WorkspaceGroup, WorkspaceGroupProperty, ITableWorkspaceProperty,
                        Progress, PropertyMode)
from mantid.kernel import Direction
from mantid.simpleapi import *


class SANSSuperImpose(PythonAlgorithm):

    def category(self):
        return 'SANS'

    def name(self):
        return "SANSSuperImpose"

    def summary(self):
        return "From a Workspace Group of I(Q) calculates I(scaled) = f * I(original) – b"

    def PyInit(self):
        # In
        self.declareProperty(
            WorkspaceGroupProperty(
                "InputWorkspaceGroup", "",
                direction=Direction.Input),
            doc="I(q, wavelength) non-scaled workspaces.")
        # Out
        self.declareProperty(
            WorkspaceGroupProperty(
                "OutputWorkspaceGroup", "",
                direction=Direction.Output,
                optional=PropertyMode.Optional),
            doc="I(q, wavelength) scaled workspaces")
        self.declareProperty(
            ITableWorkspaceProperty(
                'OutputWorkspaceTable', '',
                optional=PropertyMode.Optional,
                direction=Direction.Output),
            doc='Table workspace of fit parameters')

    def PyExec(self):

        input_workspaces = self.getProperty('InputWorkspaceGroup').value
        input_workspace_names = input_workspaces.getNames()

        progress = Progress(self, 0.0, 0.05, 3)        
        for input_ws in input_workspace_names:
            logger.information('Running Fitting for workspace: %s' % input_ws)
            progress.report('Running Fitting for workspace: %s' % input_ws)






        # create output table ws
        outws_name = self.getPropertyValue("OutputWorkspaceTable")

        
        outws = CreateEmptyTableWorkspace(OutputWorkspace=outws_name)
        columns = ["IQCurve", "K", "KError", "B", "BError"]
        
        outws.addColumn(type="str", name=columns[0])
        for col in columns[1:]:
            outws.addColumn(type="double", name=col)
        
        row = {
            "IQCurve": 'xpto', 
            "K": 1,
            "KError": 0.1,
            "B": 2,
            "BError": 0.2
        }
        outws.addRow(row)
        self.setProperty("OutputWorkspaceTable", outws)
        return
        

##########################################################################


AlgorithmFactory.subscribe(SANSSuperImpose)
