#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, AlgorithmFactory)
from mantid.simpleapi import *
import numpy as np

class IndirectNormSpectra(DataProcessorAlgorithm):

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

        num_hists = self._input_ws.getNumberHistograms()
        output_ws = mtd[self._output_ws_name]

        for idx in range(num_hists):
            y_data = self._input_ws.readY(idx)
            ymax = np.amax(y_data)
            y_data = y_data/ymax
            output_ws.setY(idx, y_data)
            e_data = self._input_ws.readE(idx)
            e_data = e_data/ymax
            output_ws.setE(idx, e_data)

        self.setProperty('OutputWorkspace', output_ws)

#----------------------------------Helper Functions---------------------------------




    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._input_ws = mtd[self._input_ws_name]
        self._output_ws_name= self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectNormSpectra)
