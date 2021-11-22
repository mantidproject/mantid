# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import (AlgorithmFactory, DataProcessorAlgorithm, MatrixWorkspaceProperty, MatrixWorkspace,
                        PropertyMode, WorkspaceGroup, WorkspaceProperty)
from mantid.kernel import (CompositeValidator, StringArrayLengthValidator, StringArrayMandatoryValidator,
                           StringArrayProperty, Direction)


class ReflectometryISISPreprocess(DataProcessorAlgorithm):
    _RUNS = 'InputRunList'
    _GROUP_TOF = 'GroupTOFWorkspaces'
    _OUTPUT_WS = 'OutputWorkspace'
    _MONITOR_WS = 'MonitorWorkspace'
    _EVENT_MODE = "EventMode"

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)

    def category(self):
        """Return the categories of the algorithm."""
        return 'Reflectometry\\ISIS;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryISISPreprocess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Preprocess ISIS reflectometry data, including optional loading and summing of the input runs."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometryISISLoadAndProcess', 'ReflectometryReductionOneAuto']

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty(self._RUNS, values=[], validator=self._get_input_runs_validator()),
            doc='A list of run numbers or workspace names to load and preprocess')
        self.declareProperty(self._EVENT_MODE, False, direction=Direction.Input,
                             doc='If true, load the input workspaces as event data')
        self.declareProperty(
            WorkspaceProperty(self._OUTPUT_WS, '', direction=Direction.Output),
            doc='The preprocessed output workspace. If multiple input runs are specified '
                'they will be summed into a single output workspace.')
        self.declareProperty(
            MatrixWorkspaceProperty(self._MONITOR_WS, '', direction=Direction.Output, optional=PropertyMode.Optional),
            doc='The loaded monitors workspace. This is only output in event mode.')

    def PyExec(self):
        workspace, monitor_ws = self._loadRun(self.getPropertyValue(self._RUNS))
        self.setProperty(self._OUTPUT_WS, workspace)
        if monitor_ws:
            self.setProperty(self._MONITOR_WS, monitor_ws)

    @staticmethod
    def _get_input_runs_validator():
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        lenValidator = StringArrayLengthValidator()
        lenValidator.setLengthMin(1)
        mandatoryInputRuns.add(lenValidator)
        return mandatoryInputRuns

    def _loadRun(self, run: str) -> MatrixWorkspace:
        """Load a run as an event workspace if slicing is requested, or a histogram
        workspace otherwise. Transmission runs are always loaded as histogram workspaces."""
        event_mode = self.getProperty(self._EVENT_MODE).value
        monitor_ws = None
        if event_mode:
            alg = self.createChildAlgorithm('LoadEventNexus', Filename=run, LoadMonitors=True)
            alg.execute()
            ws = alg.getProperty('OutputWorkspace').value
            monitor_ws = alg.getProperty('MonitorWorkspace').value
            self._validate_event_ws(ws)
            self.log().information('Loaded event workspace')
        else:
            alg = self.createChildAlgorithm('LoadNexus', Filename=run)
            alg.execute()
            ws = alg.getProperty('OutputWorkspace').value
            self.log().information('Loaded workspace ')
        return ws, monitor_ws

    @staticmethod
    def _validate_event_ws(workspace):
        if isinstance(workspace, WorkspaceGroup):
            # Our reduction algorithm doesn't currently support this due to slicing
            # (which would result in a group of groups)
            raise RuntimeError('Loading Workspace Groups in event mode is not supported currently.')
        if not workspace.run().hasProperty('proton_charge'):
            # Reduction algorithm requires proton_charge
            raise RuntimeError('Event workspaces must contain proton_charge')


AlgorithmFactory.subscribe(ReflectometryISISPreprocess)
