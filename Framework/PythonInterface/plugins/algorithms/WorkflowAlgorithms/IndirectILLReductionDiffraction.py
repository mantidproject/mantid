# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import PythonAlgorithm, MatrixWorkspaceProperty, MultipleFileProperty, PropertyMode, Progress, \
    WorkspaceGroupProperty, FileAction
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator
from mantid.simpleapi import *


class IndirectILLReductionDiffraction(PythonAlgorithm):
    """
    Performs reduction on IN16B's diffraction data. It can be on mode Doppler or BATS.
    """

    runs = None
    mode = None
    output = None
    progress = None

    def category(self):
        return "ILL\\Indirect"

    def summary(self):
        return "Performs reduction on IN16B's diffraction data. Mode is either Doppler or BATS."

    def name(self):
        return "IndirectILLReductionDiffraction"

    def setUp(self):
        self.runs = self.getPropertyValue('SampleRuns').split(',')
        self.mode = self.getPropertyValue('Mode')
        self.output = self.getPropertyValue('OutputWorkspace')
        self.progress = Progress(self, start=0.0, end=1.0, nreports=10)

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('SampleRuns', extensions=['nxs']), doc="File path for run(s).")

        self.declareProperty(name='Mode',
                             defaultValue='Doppler',
                             validator=StringListValidator(['Doppler', 'BATS']),
                             doc='Diffraction mode used.')

        # TODO find if necessary, and if so, what it is (and change doc)
        self.declareProperty("EFixed", 0., validator=FloatBoundedValidator(lower=0.),
                             doc="I don't know what this is but ConvertSpectrumAxis asks for it")

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '', direction=Direction.Output),
                             doc='The output workspace group containing reduced data.')

    def _normalize_by_monitor(self, ws):
        """
            Normalizes the workspace by monitor values (ID is 0 for IN16B)
            @param ws : the input workspace
        """
        monitorID = 0
        monitor_ws = ws + '_mon'
        ExtractSpectra(InputWorkspace=ws, DetectorList=monitorID, OutputWorkspace=monitor_ws)
        # if mtd[monitor_ws].readY(0)[0] == 0:
        #     raise RuntimeError('Normalise to monitor requested, but monitor has 0 counts.')
        # else:
        Divide(LHSWorkspace=ws, RHSWorkspace=monitor_ws, OutputWorkspace=ws, WarnOnZeroDivide=True)
        DeleteWorkspace(monitor_ws)

    def _treat_doppler(self, ws):

        self._normalize_by_monitor(ws)

        number_of_channels = mtd[ws].blocksize()
        Rebin(InputWorkspace=ws, OutputWorkspace=self.output, Params=[0, number_of_channels, number_of_channels])

        # TODO find real value (and what it means)
        e_fixed = 5

        ConvertSpectrumAxis(InputWorkspace=self.output,
                            OutputWorkspace=self.output,
                            Target='ElasticQ',
                            EMode="Indirect",
                            EFixed=e_fixed)

        self.setProperty("OutputWorkspace", mtd[self.output])

    def _treat_BATS(self, ws):
        pass

    def PyExec(self):
        self.setUp()
        LoadILLIndirect(Filename=self.getPropertyValue('SampleRuns'), OutputWorkspace=self.output,
                        LoadDiffractionData=True)

        if self.mode == "Doppler":
            self._treat_doppler(self.output)
        elif self.mode == "BATS":
            self._treat_BATS(self.output)


AlgorithmFactory.subscribe(IndirectILLReductionDiffraction)
