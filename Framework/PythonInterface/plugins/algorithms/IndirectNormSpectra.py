#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import (WorkspaceProperty, FileProperty, FileAction, TextAxis,
                        DataProcessorAlgorithm, AlgorithmFactory, mtd)
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
        self.declareProperty(WorkspaceProperty('InputWorkspace', '',
                                               direction=Direction.Input),
                             doc='Input workspace')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace')

    def validateInputs(self):
        self._setup()
        issues = dict()

        # Ensure there is atleast 1 histogram
        num_hists = self._input_ws.getNumberHistograms()
        if num_hists == 0:
            issues['InputWorkspace'] = 'InputWorkspace must have atleast 1 histogram'

    def PyExec(self):

        CloneWorkspace(InputWorkspace=self._input_ws,
                       OutputWorkspace=self._output_ws_name)
        ws_out = mtd[self._output_ws_name]

        for idx in range(num_hist):
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
        self._input_ws = self.getProperty('InputWorkspace')
        self._output_ws_name = self.getPropertyValue('OutputWorkspace')

# Register algorithm with Mantid
AlgorithmFactory.subscribe(IndirectNormSpectra)
