# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
from mantid.api import mtd, MatrixWorkspaceProperty, DataProcessorAlgorithm, AlgorithmFactory
from mantid.kernel import Direction
from mantid.simpleapi import CloneWorkspace, DeleteWorkspace, ExtractSpectra, Scale

import numpy as np


class NormaliseSpectra(DataProcessorAlgorithm):
    _input_ws_name = None
    _input_ws = None
    _output_ws_name = None

    def category(self):
        return "Workflow\\MIDAS;Inelastic"

    def summary(self):
        return "Normalise all spectra to have a max value of 1"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input), doc="Input workspace")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Output workspace")

    def PyExec(self):
        self._setup()
        CloneWorkspace(InputWorkspace=self._input_ws, OutputWorkspace=self._output_ws_name)

        num_hists = self._input_ws.getNumberHistograms()
        output_ws = mtd[self._output_ws_name]

        for idx in range(num_hists):
            single_spectrum = ExtractSpectra(InputWorkspace=self._input_ws, WorkspaceIndexList=idx)
            y_data = single_spectrum.readY(0)
            ymax = np.nanmax(y_data)
            # raises a RuntimeError if the ymax is <= 0
            if ymax <= 0:
                spectrum_no = single_spectrum.getSpectrum(0).getSpectrumNo()
                DeleteWorkspace("single_spectrum")
                raise RuntimeError(
                    "Spectrum number %d:" % (spectrum_no)
                    + " has a maximum y value of 0 or less. "
                    + "All spectra must have a maximum y value more than 0"
                )
            Scale(InputWorkspace=single_spectrum, Operation="Multiply", Factor=(1 / ymax), OutputWorkspace=single_spectrum)
            output_ws.setY(idx, single_spectrum.readY(0))
            output_ws.setE(idx, single_spectrum.readE(0))

        # Delete extracted spectra workspace
        DeleteWorkspace("single_spectrum")

        self.setProperty("OutputWorkspace", output_ws)

    def _setup(self):
        """
        Gets properties.
        """

        self._input_ws_name = self.getPropertyValue("InputWorkspace")
        self._input_ws = mtd[self._input_ws_name]
        self._output_ws_name = self.getPropertyValue("OutputWorkspace")


# Register algorithm with Mantid
AlgorithmFactory.subscribe(NormaliseSpectra)
