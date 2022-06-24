# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, FileAction, FileProperty, \
    MultipleFileProperty, Progress, PythonAlgorithm, WorkspaceGroupProperty
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator
from mantid.simpleapi import *


class D4ILLReduction(PythonAlgorithm):

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs diffraction data reduction for the D4 instrument at the ILL.'

    def seeAlso(self):
        return ['LoadILLDiffraction']

    def name(self):
        return 'D4ILLReduction'

    def validateInputs(self):
        issues = dict()

        return issues

    def PyInit(self):

        self.declareProperty(MultipleFileProperty('Run', extensions=['nxs']),
                             doc='File path of run(s).')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', '',
                                                    direction=Direction.Output),
                             doc='The output workspace based on the value of ProcessAs.')

        self.declareProperty(FileProperty('BankPositionOffsetsFile', '',
                                          action=FileAction.OptionalLoad),
                             doc='The path to the file with bank position offsets.')

        self.declareProperty(name="ZeroPositionAngle",
                             defaultValue=0.0,
                             doc="The angular position correction common to all banks.")

        self.declareProperty(FileProperty('EfficiencyCalibrationFile', '',
                                          action=FileAction.OptionalLoad),
                             doc='The path to the file with relative detector efficiencies.')

        self.declareProperty(name="DeadTimeTauDetector",
                             defaultValue=7e-6,
                             doc="The count rate coefficient for detectors.")

        self.declareProperty(name="DeadTimeTauMonitor",
                             defaultValue=2.4e-6,
                             doc="The count rate coefficient for the monitor.")

        self.declareProperty(name="Wavelength",
                             defaultValue=0.5,
                             doc="The measurement wavelength, in Angstrom.")

        self.declareProperty(name="NormaliseBy",
                             defaultValue="Monitor",
                             validator=StringListValidator(["Monitor", "Time", "None"]),
                             direction=Direction.Input,
                             doc="What normalisation approach to use on data.")

        self.declareProperty(name="NormalisationStandard",
                             defaultValue=1.e6,
                             doc="Standard value against which the normalisation which be performed. The default is for"
                                 " normalisation to monitor.")

        self.declareProperty(name="ScatteringAngleBinSize",
                             defaultValue=0.5,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="Scattering angle bin size in degrees used for expressing scan data on a single"
                                 " TwoTheta axis.")

        self.declareProperty('ExportASCII', True,
                             doc='Whether or not to export the output workspaces as ASCII files.')

        self.declareProperty('ClearCache', True,
                             doc='Whether or not to clear the cache of intermediate workspaces.')

        self.declareProperty('DebugMode',
                             defaultValue=False,
                             doc="Whether to create and show all intermediate workspaces at each correction step.")

    def _correct_bank_positions(self, ws):
        return ws

    def _correct_dead_time(self, ws, tau):
        return ws

    def _correct_relative_efficiency(self, ws):
        return ws

    def _create_diffractograms(self, ws):
        return ws

    def _finalize(self, ws):
        """Finalizes the reduction step by removing special values, calling merging functions and setting unique names
         to the output workspaces."""
        RenameWorkspace(InputWorkspace=ws, OutputWorkspace=ws[2:])
        ws = ws[2:]
        self.setProperty('OutputWorkspace', mtd[ws])

    def _normalise(self, ws, mon):
        return ws

    def _load_data(self, progress):
        """Loads the data scan, with each scan step in individual workspace, and splits detectors from monitors."""
        ws = '__' + self.getPropertyValue('OutputWorkspace')
        progress.report(0, 'Loading data')
        LoadAndMerge(Filename=self.getPropertyValue('Run'), LoaderName='LoadILLDiffraction',
                     OutputWorkspace=ws, startProgress=0.0, endProgress=0.3)
        mon = ws + '_mon'
        mon_list = []
        det_list = []
        for entry in mtd[ws]:
            mon_ws = entry.name() + '_mon'
            mon_list.append(mon_ws)
            det_ws = entry.name()
            det_list.append(det_ws)
            ExtractMonitors(InputWorkspace=entry, DetectorWorkspace=det_ws,
                            MonitorWorkspace=mon_ws)
        GroupWorkspaces(InputWorkspaces=mon_list, OutputWorkspace=mon)
        GroupWorkspaces(InputWorkspaces=det_list, OutputWorkspace=ws)
        return ws, mon, progress

    def PyExec(self):
        nReports = 7
        progress = Progress(self, start=0.0, end=1.0, nreports=nReports)
        self._debug = self.getProperty('DebugMode').value

        ws, mon, progress = self._load_data(progress)
        progress.report('Correcting for dead-time')
        ws = self._correct_dead_time(ws, self.getProperty("DeadTimeTauDetector").value)
        mon = self._correct_dead_time(mon, self.getProperty("DeadTimeTauMonitor").value)
        progress.report('Correcting bank positions')
        ws = self._correct_bank_positions(ws)
        progress.report('Correcting relative efficiency')
        ws = self._correct_relative_efficiency(ws)
        progress.report('Normalising to monitor/time')
        ws = self._normalise(ws, mon)
        progress.report('Creating diffractograms')
        ws = self._create_diffractograms(ws)
        progress.report('Finalizing')
        self._finalize(ws)


AlgorithmFactory.subscribe(D4ILLReduction)
