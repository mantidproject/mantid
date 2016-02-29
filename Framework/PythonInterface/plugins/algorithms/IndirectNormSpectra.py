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

        ws_in = mtd[self._input_ws_name]
        CloneWorkspace(InputWorkspace=self._input_ws_name,
                       OutputWorkspace=self._output_ws_name)

        ws_out = mtd[self._output_ws_name]
        num_hists = ws_in.getNumberHistograms()

        for idx in range(num_hists):
            y = ws_in.readY(idx)
            ymax = np.amax(y)
            y = y/ymax
            ws_out.setY(idx, y)
            e = ws_in.readE(idx)
            e = e/ymax
            ws_out.setE(idx, e)


        self.setProperty('OutputWorkspace', ws_out)


    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws_name = self.getPropertyValue('InputWorkspace')
        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectNormSpectra)
