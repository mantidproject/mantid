#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (WorkspaceProperty, FileProperty, FileAction, TextAxis,
                        DataProcessorAlgorithm, AlgorithmFactory, mtd)
from mantid.simpleapi import *
import numpy as np

class IndirectNormSpectra(DataProcessorAlgorithm):

    _input_ws = None
    _output_ws = None


    def category(self):
        return 'Workflow\\MIDAS;PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Normalise all spectra to have a max value of 1'


    def PyInit(self):
        self.declareProperty(WorkspaceProperty('InputWorkspace', '',
                                               direction=Direction.Input),
                             doc='Input workspace')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace')


    def PyExec(self):
        self._setup()

        ws_in = mtd[self._input_ws]
        num_hist = ws_in.getNumberHistograms()  # no. of hist/groups in WS
        if num_hist == 0:
            raise ValueError('Workspace %s has NO histograms' % self._input_ws)

        CloneWorkspace(InputWorkspace=self._input_ws,
                       OutputWorkspace=self._output_ws)
        ws_out = mtd[self._output_ws]

        for idx in range(num_hist):
            y = ws_in.readY(idx)
            ymax = np.amax(y)
            y = y/ymax
            ws_out.setY(idx, y)
            e = ws_in.readE(idx)
            e = e/ymax
            ws_out.setE(idx, e)


        self.setProperty('OutputWorkspace', self._output_ws)


    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._output_ws = self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectNormSpectra)
