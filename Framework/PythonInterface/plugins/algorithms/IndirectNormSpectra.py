#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (MatrixWorkspaceProperty, DataProcessorAlgorithm, AlgorithmFactory)
from mantid.simpleapi import *
import numpy as np

class IndirectNormSpectra(DataProcessorAlgorithm):

    _input_ws_name = None
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
        input_ws = mtd[self._input_ws_name]
        CloneWorkspace(InputWorkspace=input_ws,
                       OutputWorkspace=self._output_ws_name)

        num_hists = input_ws.getNumberHistograms()
        output_ws = mtd[self._output_ws_name]

        for idx in range(num_hists):
            y = input_ws.readY(idx)
            ymax = np.amax(y)
            y = y/ymax
            output_ws.setY(idx, y)
            e = input_ws.readE(idx)
            e = e/ymax
            output_ws.setE(idx, e)


        self.setProperty('OutputWorkspace', output_ws)


    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._output_ws_name= self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectNormSpectra)
