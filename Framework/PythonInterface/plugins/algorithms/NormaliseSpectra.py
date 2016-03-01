#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, AlgorithmFactory)
from mantid.simpleapi import *

import numpy as np


class NormaliseSpectra(DataProcessorAlgorithm):

    _input_ws_name = None
    _input_ws = None
    _output_ws_name = None


    def category(self):
        return 'Workflow\\MIDAS;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Normalise all spectra to have a max value of 1'


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace', '', direction=Direction.Input),
                                                     doc='Input workspace')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',direction=Direction.Output),
                                                     doc='Output workspace')


    def PyExec(self):
        self._setup()
        CloneWorkspace(InputWorkspace=self._input_ws,
                       OutputWorkspace=self._output_ws_name)
        # Clone input workspace to incase it requires scaling
        ws_in = CloneWorkspace(InputWorkspace=self._input_ws)

        num_hists = self._input_ws.getNumberHistograms()
        output_ws = mtd[self._output_ws_name]
        self._input_ws = self._scale_negative_and_zero_data(ws_in)

        for idx in range(num_hists):
            y_data = self._input_ws.readY(idx)
            ymax = np.amax(y_data)
            y_data = y_data/ymax
            output_ws.setY(idx, y_data)
            e_data = self._input_ws.readE(idx)
            e_data = e_data/ymax
            output_ws.setE(idx, e_data)

        # Delete cloned input workspace
        DeleteWorkspace('ws_in')

        output_ws = self._replace_nan_with_zero(output_ws)

        self.setProperty('OutputWorkspace', output_ws)

#----------------------------------Helper Functions---------------------------------

    def _scale_negative_and_zero_data(self, workspace):
        """
        Checks for negative data in workspace and scales workspace if data is negative
        """
        num_hists = workspace.getNumberHistograms()
        lowest_value = 0.0
        for hist in range(num_hists):
            y_data = workspace.readY(hist)
            ymin = np.amin(y_data)
            if ymin < lowest_value:
                lowest_value = ymin
        if lowest_value <= 0.0:
            logger.warning('Scaling Workspace to remove negative data')
            scale_factor = 0-lowest_value
            Scale(InputWorkspace=workspace, OutputWorkspace=workspace,
                  Factor=scale_factor, Operation='Add')
        return workspace

    def _replace_nan_with_zero(self, workspace):
        nhists = workspace.getNumberHistograms()
        for hist in range(nhists):
            y_data = workspace.readY(hist)
            workspace.setY(hist, np.nan_to_num(y_data))
        return workspace

    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._input_ws = mtd[self._input_ws_name]
        self._output_ws_name= self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(NormaliseSpectra)
