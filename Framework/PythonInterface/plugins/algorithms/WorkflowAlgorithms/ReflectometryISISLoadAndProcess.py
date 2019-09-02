# -*- coding: utf-8 -*-
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from mantid.api import (AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm,
                        PropertyMode, WorkspaceGroup, WorkspaceProperty)

from mantid.simpleapi import (LoadEventNexus, LoadNexus, MergeRuns, RenameWorkspace)

from mantid.kernel import (CompositeValidator, Direction, EnabledWhenProperty,
                           IntBoundedValidator, Property, PropertyCriterion,
                           StringArrayLengthValidator, StringArrayMandatoryValidator,
                           StringArrayProperty)


class Prop:
    RUNS = 'InputRunList'
    FIRST_TRANS_RUNS = 'FirstTransmissionRunList'
    SECOND_TRANS_RUNS = 'SecondTransmissionRunList'
    SLICE = 'SliceWorkspace'
    NUMBER_OF_SLICES = 'NumberOfSlices'
    QMIN='MomentumTransferMin'
    QSTEP='MomentumTransferStep'
    QMAX='MomentumTransferMax'
    GROUP_TOF='GroupTOFWorkspaces'
    RELOAD='ReloadInvalidWorkspaces'
    OUTPUT_WS='OutputWorkspace'
    OUTPUT_WS_BINNED='OutputWorkspaceBinned'
    OUTPUT_WS_LAM='OutputWorkspaceWavelength'


class ReflectometryISISLoadAndProcess(DataProcessorAlgorithm):

    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)
        self._tofPrefix = "TOF_"
        self._transPrefix = "TRANS_"

    def category(self):
        """Return the categories of the algorithm."""
        return 'ISIS\\Reflectometry;Workflow\\Reflectometry'

    def name(self):
        """Return the name of the algorithm."""
        return 'ReflectometryISISLoadAndProcess'

    def summary(self):
        """Return a summary of the algorithm."""
        return "Reduce ISIS reflectometry data, including optional loading and summing/slicing of the input runs."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ['ReflectometrySliceEventWorkspace', 'ReflectometryReductionOneAuto']

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        lenValidator = StringArrayLengthValidator()
        lenValidator.setLengthMin(1)
        mandatoryInputRuns.add(lenValidator)
        self.declareProperty(StringArrayProperty(Prop.RUNS,
                                                 values=[],
                                                 validator=mandatoryInputRuns),
                             doc='A list of run numbers or workspace names for the input runs. '
                             'Multiple runs will be summed before reduction.')
        self.declareProperty(StringArrayProperty(Prop.FIRST_TRANS_RUNS,
                                                 values=[]),
                             doc='A list of run numbers or workspace names for the first transmission run. '
                             'Multiple runs will be summed before reduction.')
        self.declareProperty(StringArrayProperty(Prop.SECOND_TRANS_RUNS,
                                                 values=[]),
                             doc='A list of run numbers or workspace names for the second transmission run. '
                             'Multiple runs will be summed before reduction.')
        self._declareSliceAlgorithmProperties()
        self._declareReductionAlgorithmProperties()
        self.declareProperty(Prop.GROUP_TOF, True, doc = 'If true, group the input TOF workspace')
        self.declareProperty(Prop.RELOAD, True, doc = 'If true, reload input workspaces if they are of the incorrect type')
        self.declareProperty(WorkspaceProperty(Prop.OUTPUT_WS, '',
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             doc='The output workspace, or workspace group if sliced.')
        self.declareProperty(WorkspaceProperty(Prop.OUTPUT_WS_BINNED, '',
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             doc='The binned output workspace, or workspace group if sliced.')
        self.declareProperty(WorkspaceProperty(Prop.OUTPUT_WS_LAM, '',
                                               optional=PropertyMode.Optional,
                                               direction=Direction.Output),
                             doc='The output workspace in wavelength, or workspace group if sliced.')

    def PyExec(self):
        """Execute the algorithm."""
        self._reload = self.getProperty(Prop.RELOAD).value
        # Convert run numbers to real workspaces
        inputRuns = self.getProperty(Prop.RUNS).value
        inputWorkspaces = self._getInputWorkspaces(inputRuns, False)
        firstTransRuns = self.getProperty(Prop.FIRST_TRANS_RUNS).value
        firstTransWorkspaces = self._getInputWorkspaces(firstTransRuns, True)
        secondTransRuns = self.getProperty(Prop.SECOND_TRANS_RUNS).value
        secondTransWorkspaces = self._getInputWorkspaces(secondTransRuns, True)
        # Combine multiple input runs, if required
        input_workspace = self._sumWorkspaces(inputWorkspaces, False)
        first_trans_workspace = self._sumWorkspaces(firstTransWorkspaces, True)
        second_trans_workspace = self._sumWorkspaces(secondTransWorkspaces, True)
        # Slice the input workspace, if required
        input_workspace = self._sliceWorkspace(input_workspace)
        # Perform the reduction
        alg = self._reduce(input_workspace, first_trans_workspace, second_trans_workspace)
        self._finalize(alg)
        if len(inputWorkspaces) >= 2:
            inputWorkspaces.append(input_workspace)
        self._group_workspaces(inputWorkspaces, "TOF")

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if len(self.getProperty(Prop.RUNS).value) > 1 and self.getProperty(Prop.SLICE).value:
            issues[Prop.SLICE] = "Cannot perform slicing when summing multiple input runs"
        return issues

    def _declareSliceAlgorithmProperties(self):
        """Copy properties from the child slicing algorithm and add our own custom ones"""
        self.declareProperty(Prop.SLICE, False, doc = 'If true, slice the input workspace')
        whenSliceEnabled = EnabledWhenProperty(Prop.SLICE, PropertyCriterion.IsEqualTo, "1")

        self._slice_properties = ['TimeInterval', 'LogName', 'LogValueInterval']
        self.copyProperties('ReflectometrySliceEventWorkspace', self._slice_properties)
        for property in self._slice_properties:
            self.setPropertySettings(property, whenSliceEnabled)

        self.declareProperty(name=Prop.NUMBER_OF_SLICES,
                             defaultValue=Property.EMPTY_INT,
                             validator=IntBoundedValidator(lower=1),
                             direction=Direction.Input,
                             doc='The number of uniform-length slices to slice the input workspace into')
        self.setPropertySettings(Prop.NUMBER_OF_SLICES, whenSliceEnabled)

    def _declareReductionAlgorithmProperties(self):
        """Copy properties from the child reduction algorithm"""
        self._reduction_properties = [
            'ThetaIn', 'ThetaLogName',
            'SummationType', 'ReductionType', 'IncludePartialBins',
            'AnalysisMode', 'ProcessingInstructions', 'CorrectDetectors',
            'DetectorCorrectionType', 'WavelengthMin', 'WavelengthMax', 'I0MonitorIndex',
            'MonitorBackgroundWavelengthMin', 'MonitorBackgroundWavelengthMax',
            'MonitorIntegrationWavelengthMin', 'MonitorIntegrationWavelengthMax',
            'NormalizeByIntegratedMonitors', 'Params', 'StartOverlap', 'EndOverlap',
            'ScaleRHSWorkspace', 'TransmissionProcessingInstructions', 'CorrectionAlgorithm', 'Polynomial', 'C0', 'C1',
            'MomentumTransferMin', 'MomentumTransferStep', 'MomentumTransferMax', 'ScaleFactor',
            'PolarizationAnalysis', 'FloodCorrection',
            'FloodWorkspace', 'Debug']
        self.copyProperties('ReflectometryReductionOneAuto', self._reduction_properties)

    def _getInputWorkspaces(self, runs, isTrans):
        """Convert the given run numbers into real workspace names. Uses workspaces from
        the ADS if they exist, or loads them otherwise."""
        workspaces = list()
        for run in runs:
            ws = self._getRunFromADSOrNone(run, isTrans)
            if not ws:
                ws = self._loadRun(run, isTrans)
            if not ws:
                raise RuntimeError('Error loading run ' + run)
            workspaces.append(ws)
        return workspaces

    def _prefixedName(self, name, isTrans):
        """Add a prefix for TOF workspaces onto the given name"""
        if isTrans:
            return self._transPrefix + name
        else:
            return self._tofPrefix + name

    def _isValidWorkspace(self, workspace_name, workspace_id):
        """Returns true, if the workspace of name workspace_name is a valid
        reflectometry workspace of type workspace_id. Otherwise, deletes the
        workspace if the user requested to reload invalid workspaces, or raises
        an error otherwise
        """
        if not _hasWorkspaceID(workspace_name, workspace_id):
            message = 'Workspace ' + workspace_name + ' exists but is not a ' + workspace_id
            if self._reload:
                self.log().information(message)
                _removeWorkspace(workspace_name)
                return False
            else:
                raise RuntimeError(message)

        # For event workspaces, the monitors workspace must also exist, otherwise it's not valid
        if workspace_id == "EventWorkspace":
            if not AnalysisDataService.doesExist(_monitorWorkspace(workspace_name)):
                if self._reload:
                    message = 'Monitors workspace ' + workspace_name + '_monitors does not exist'
                    self.log().information(message)
                    _removeWorkspace(workspace_name)
                    return False
                else:
                    raise RuntimeError(message)
        return True

    def _workspaceExistsAndIsValid(self, workspace_name, isTrans):
        """Return true, if the given workspace exists in the ADS and is valid"""
        if not AnalysisDataService.doesExist(workspace_name):
            self.log().information('Workspace ' + workspace_name + ' does not exist')
            return False
        self.log().information('Workspace ' + workspace_name + ' exists')
        if not isTrans and self._slicingEnabled():
            return self._isValidWorkspace(workspace_name, "EventWorkspace")
        else:
            return self._isValidWorkspace(workspace_name, "Workspace2D")

    def _getRunFromADSOrNone(self, run, isTrans):
        """Given a run name, return the name of the equivalent workspace in the ADS (
        which may or may not have a prefix applied). Returns None if the workspace does
        not exist or is not valid for the requested reflectometry reduction."""
        # Try given run number
        workspace_name = run
        if self._workspaceExistsAndIsValid(workspace_name, isTrans):
            return workspace_name
        # Try with prefix
        workspace_name = self._prefixedName(run, isTrans)
        if self._workspaceExistsAndIsValid(workspace_name, isTrans):
            return workspace_name
        # Not found
        return None

    def _group_workspaces(self, workspaces, output_ws_name):
        """
        Groups all the given workspaces into a group with the given name. If the group
        already exists it will add them to that group.
        """
        if not self.getProperty(Prop.GROUP_TOF).value:
            return

        if AnalysisDataService.doesExist(output_ws_name):
            ws_group = AnalysisDataService.retrieve(output_ws_name)
            if not isinstance(ws_group, WorkspaceGroup):
                raise RuntimeError('Cannot group TOF workspaces, a workspace called TOF already exists')
            else:
                for ws in workspaces:
                    if ws not in ws_group:
                        ws_group.add(ws)
        else:
            alg = self.createChildAlgorithm("GroupWorkspaces")
            alg.setProperty("InputWorkspaces", workspaces)
            alg.setProperty("OutputWorkspace", output_ws_name)
            alg.execute()
            ws_group = alg.getProperty("OutputWorkspace").value
        AnalysisDataService.addOrReplace(output_ws_name, ws_group)
        return ws_group

    def _renameWorkspaceBasedOnRunNumber(self, workspace_name, isTrans):
        """Rename the given workspace based on its run number and a standard prefix"""
        new_name = self._prefixedName(_getRunNumberAsString(workspace_name), isTrans)
        if new_name != workspace_name:
            RenameWorkspace(InputWorkspace=workspace_name, OutputWorkspace=new_name)
            # Also rename the monitor workspace, if there is one
            if AnalysisDataService.doesExist(_monitorWorkspace(workspace_name)):
                RenameWorkspace(InputWorkspace=_monitorWorkspace(workspace_name),
                                OutputWorkspace=_monitorWorkspace(new_name))
        return new_name

    def _loadRun(self, run, isTrans):
        """Load a run as an event workspace if slicing is requested, or a histogram
        workspace otherwise. Transmission runs are always loaded as histogram workspaces."""
        workspace_name=self._prefixedName(run, isTrans)
        if not isTrans and self._slicingEnabled():
            LoadEventNexus(Filename=run, OutputWorkspace=workspace_name, LoadMonitors=True)
            _throwIfNotValidReflectometryEventWorkspace(workspace_name)
            self.log().information('Loaded event workspace ' + workspace_name)
        else:
            LoadNexus(Filename=run, OutputWorkspace=workspace_name)
            self.log().information('Loaded workspace ' + workspace_name)
        workspace_name = self._renameWorkspaceBasedOnRunNumber(workspace_name, isTrans)
        return workspace_name

    def _sumWorkspaces(self, workspaces, isTrans):
        """If there are multiple input workspaces, sum them and return the result. Otherwise
        just return the single input workspace, or None if the list is empty."""
        if len(workspaces) < 1:
            return None
        if len(workspaces) < 2:
            return workspaces[0]
        workspaces_without_prefixes = [self._removePrefix(ws, isTrans) for ws in workspaces]
        concatenated_names = "+".join(workspaces_without_prefixes)
        summed = self._prefixedName(concatenated_names, isTrans)
        self.log().information('Summing workspaces' + " ".join(workspaces) + ' into ' + summed)
        MergeRuns(InputWorkspaces=", ".join(workspaces), OutputWorkspace=summed)
        # The reduction algorithm sets the output workspace names from the run number,
        # which by default is just the first run. Set it to the concatenated name,
        # e.g. 13461+13462
        ws = AnalysisDataService.retrieve(summed)
        if isinstance(ws, WorkspaceGroup):
            for workspaceName in ws.getNames():
                grouped_ws = AnalysisDataService.retrieve(workspaceName)
                grouped_ws.run().addProperty('run_number', concatenated_names, True)
        else:
            ws.run().addProperty('run_number', concatenated_names, True)
        return summed

    def _slicingEnabled(self):
        return self.getProperty(Prop.SLICE).value

    def _setUniformNumberOfSlices(self, alg, workspace_name):
        """If slicing by a specified number of slices is requested, find the time
        interval to use to give this number of even time slices and set the relevant
        property on the given slicing algorithm"""
        if self.getProperty(Prop.NUMBER_OF_SLICES).isDefault:
            return
        number_of_slices = self.getProperty(Prop.NUMBER_OF_SLICES).value
        run=AnalysisDataService.retrieve(workspace_name).run()
        total_duration = (run.endTime() - run.startTime()).total_seconds()
        slice_duration = total_duration / number_of_slices
        alg.setProperty("TimeInterval", slice_duration)
        self.log().information('Slicing ' + workspace_name + ' into ' + str(number_of_slices)
                               + ' even slices of duration ' + str(slice_duration))

    def _setSliceStartStopTimes(self, alg, workspace_name):
        """Set the start/stop time for the slicing algorithm based on the
        run start/end times if the time interval is specified, otherwise
        we can end up with more slices than we expect"""
        if alg.getProperty("TimeInterval").isDefault:
            return
        run=AnalysisDataService.retrieve(workspace_name).run()
        alg.setProperty("StartTime", str(run.startTime()))
        alg.setProperty("StopTime", str(run.endTime()))

    def _runSliceAlgorithm(self, input_workspace, output_workspace):
        """Run the child algorithm to perform the slicing"""
        self.log().information('Running ReflectometrySliceEventWorkspace')
        alg = self.createChildAlgorithm("ReflectometrySliceEventWorkspace")
        for property in self._slice_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        alg.setProperty("OutputWorkspace", output_workspace)
        alg.setProperty("InputWorkspace", input_workspace)
        alg.setProperty("MonitorWorkspace", _monitorWorkspace(input_workspace))
        self._setUniformNumberOfSlices(alg, input_workspace)
        self._setSliceStartStopTimes(alg, input_workspace)
        alg.execute()

    def _sliceWorkspace(self, workspace):
        """If slicing has been requested, slice the input workspace, otherwise
        return it unchanged"""
        if not self._slicingEnabled():
            return workspace
        sliced_workspace_name = self._getSlicedWorkspaceGroupName(workspace)
        self.log().information('Slicing workspace ' + workspace + ' into ' + sliced_workspace_name)
        self._runSliceAlgorithm(workspace, sliced_workspace_name)
        return sliced_workspace_name

    def _getSlicedWorkspaceGroupName(self, workspace):
        return workspace + '_sliced'

    def _setChildAlgorithmPropertyIfProvided(self, alg, property_name):
        """Set the given property on the given algorithm if it is set in our
        inputs. Leave it unset otherwise."""
        if not self.getProperty(property_name).isDefault:
            alg.setProperty(property_name, self.getPropertyValue(property_name))

    def _reduce(self, input_workspace, first_trans_workspace, second_trans_workspace):
        """Run the child algorithm to do the reduction. Return the child algorithm."""
        self.log().information('Running ReflectometryReductionOneAuto on ' + input_workspace)
        alg = self.createChildAlgorithm("ReflectometryReductionOneAuto")
        for property in self._reduction_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        self._setChildAlgorithmPropertyIfProvided(alg, Prop.OUTPUT_WS)
        self._setChildAlgorithmPropertyIfProvided(alg, Prop.OUTPUT_WS_BINNED)
        self._setChildAlgorithmPropertyIfProvided(alg, Prop.OUTPUT_WS_LAM)
        alg.setProperty("InputWorkspace", input_workspace)
        alg.setProperty("FirstTransmissionRun", first_trans_workspace)
        alg.setProperty("SecondTransmissionRun", second_trans_workspace)
        alg.execute()
        return alg

    def _removePrefix(self, workspace, isTrans):
        """Remove the TOF prefix from the given workspace name"""
        prefix = self._transPrefix if isTrans else self._tofPrefix
        prefix_len = len(prefix)
        name_start = workspace[:prefix_len]
        if len(workspace) > prefix_len and name_start == prefix:
            return workspace[prefix_len:]
        else:
            return workspace

    def _finalize(self, child_alg):
        """Set our output properties from the results in the given child algorithm"""
        self._setOutputProperty(Prop.OUTPUT_WS, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_BINNED, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_LAM, child_alg)
        self._setOutputPropertyIfInputNotSet(Prop.QMIN, child_alg)
        self._setOutputPropertyIfInputNotSet(Prop.QSTEP, child_alg)
        self._setOutputPropertyIfInputNotSet(Prop.QMAX, child_alg)

    def _setOutputProperty(self, property_name, child_alg):
        """Set the given output property from the result in the given child algorithm,
        if it exists in the child algorithm's outputs"""
        value = child_alg.getPropertyValue(property_name)
        if value:
            self.setPropertyValue(property_name, value)
            self.setProperty(property_name, child_alg.getProperty(property_name).value)

    def _setOutputPropertyIfInputNotSet(self, property_name, child_alg):
        """Set the given output property from the result in the given child algorithm,
        if it was not set as an input to this algorithm and if it exists in the
        child algorithm's outputs"""
        if self.getProperty(property_name).isDefault:
            value = child_alg.getPropertyValue(property_name)
            if value:
                self.setPropertyValue(property_name, value)
                self.setProperty(property_name, child_alg.getProperty(property_name).value)


def _throwIfNotValidReflectometryEventWorkspace(workspace_name):
    workspace=AnalysisDataService.retrieve(workspace_name)
    if isinstance(workspace, WorkspaceGroup):
        raise RuntimeError('Slicing workspace groups is not supported')
    if not workspace.run().hasProperty('proton_charge'):
        raise RuntimeError('Cannot slice workspace: run must contain proton_charge')


def _monitorWorkspace(workspace):
    """Return the associated monitor workspace name for the given workspace"""
    return workspace + '_monitors'


def _getRunNumberAsString(workspace_name):
    """Get the run number for a workspace. If it's a workspace group, get
    the run number from the first child workspace."""
    try:
        workspace = AnalysisDataService.retrieve(workspace_name)
        if not isinstance(workspace, WorkspaceGroup):
            return str(workspace.getRunNumber())
        # Get first child in the group
        return str(workspace[0].getRunNumber())
    except:
        raise RuntimeError('Could not find run number for workspace ' + workspace_name)


def _hasWorkspaceID(workspace_name, workspace_id):
    """Check that a workspace has the given type"""
    workspace = AnalysisDataService.retrieve(workspace_name)
    if isinstance(workspace, WorkspaceGroup):
        return workspace[0].id() == workspace_id
    else:
        return workspace.id() == workspace_id


def _removeWorkspace(workspace_name):
    """Remove the workspace with the given name, including any child workspaces if it
    is a group. If a corresponding monitors workspace exists, remove that too."""
    if AnalysisDataService.doesExist(workspace_name):
        workspace = AnalysisDataService.retrieve(workspace_name)
        if isinstance(workspace, WorkspaceGroup):
            # Remove child workspaces first
            while workspace.getNumberOfEntries():
                _removeWorkspace(workspace[0].name())
        AnalysisDataService.remove(workspace_name)
    # If a corresponding monitors workspace also exists, remove that too
    if AnalysisDataService.doesExist(_monitorWorkspace(workspace_name)):
        _removeWorkspace(_monitorWorkspace(workspace_name))


AlgorithmFactory.subscribe(ReflectometryISISLoadAndProcess)
