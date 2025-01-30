# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm, WorkspaceGroup

from mantid.simpleapi import MergeRuns

from mantid.kernel import (
    CompositeValidator,
    Direction,
    EnabledWhenProperty,
    IntBoundedValidator,
    Property,
    PropertyCriterion,
    StringArrayLengthValidator,
    StringArrayMandatoryValidator,
    StringArrayProperty,
)


class Prop:
    RUNS = "InputRunList"
    ROI_DETECTOR_IDS = "ROIDetectorIDs"
    FIRST_TRANS_RUNS = "FirstTransmissionRunList"
    SECOND_TRANS_RUNS = "SecondTransmissionRunList"
    SLICE = "SliceWorkspace"
    NUMBER_OF_SLICES = "NumberOfSlices"
    QMIN = "MomentumTransferMin"
    QSTEP = "MomentumTransferStep"
    QMAX = "MomentumTransferMax"
    GROUP_TOF = "GroupTOFWorkspaces"
    RELOAD = "ReloadInvalidWorkspaces"
    DEBUG = "Debug"
    HIDE_INPUT = "HideInputWorkspaces"
    OUTPUT_WS = "OutputWorkspace"
    OUTPUT_WS_BINNED = "OutputWorkspaceBinned"
    OUTPUT_WS_LAM = "OutputWorkspaceWavelength"
    OUTPUT_WS_FIRST_TRANS = "OutputWorkspaceFirstTransmission"
    OUTPUT_WS_SECOND_TRANS = "OutputWorkspaceSecondTransmission"
    OUTPUT_WS_TRANS = "OutputWorkspaceTransmission"


class ReflectometryISISLoadAndProcess(DataProcessorAlgorithm):
    def __init__(self):
        """Initialize an instance of the algorithm."""
        DataProcessorAlgorithm.__init__(self)
        self._tofPrefix = "TOF_"
        self._transPrefix = "TRANS_"

    def category(self):
        """Return the categories of the algorithm."""
        return "Reflectometry\\ISIS;Workflow\\Reflectometry"

    def name(self):
        """Return the name of the algorithm."""
        return "ReflectometryISISLoadAndProcess"

    def summary(self):
        """Return a summary of the algorithm."""
        return "Reduce ISIS reflectometry data, including optional loading and summing/slicing of the input runs."

    def seeAlso(self):
        """Return a list of related algorithm names."""
        return ["ReflectometrySliceEventWorkspace", "ReflectometryReductionOneAuto"]

    def version(self):
        """Return the version of the algorithm."""
        return 1

    def PyInit(self):
        """Initialize the input and output properties of the algorithm."""
        self._reduction_properties = []  # cached list of properties copied from child alg
        self._declareRunProperties()
        self._declarePreprocessProperties()
        self._declareSumBanksProperties()
        self._declareSlicingProperties()
        self._declareReductionProperties()
        self._declareTransmissionProperties()
        self._declareOutputProperties()
        self._declareFloodCorrectionProperties()
        self._declarePolarizationEfficiencyProperties()

    def PyExec(self):
        """Execute the algorithm."""
        if self.getProperty(Prop.HIDE_INPUT).value:
            self._tofPrefix = "__" + self._tofPrefix
            self._transPrefix = "__" + self._transPrefix
        self._reload = self.getProperty(Prop.RELOAD).value
        # Convert run numbers to real workspaces
        inputRuns = self.getProperty(Prop.RUNS).value
        inputWorkspaces = self._getInputWorkspaces(inputRuns, False)
        firstTransRuns = self.getProperty(Prop.FIRST_TRANS_RUNS).value
        firstTransWorkspaces = self._getInputWorkspaces(firstTransRuns, True)
        secondTransRuns = self.getProperty(Prop.SECOND_TRANS_RUNS).value
        secondTransWorkspaces = self._getInputWorkspaces(secondTransRuns, True)
        # Combine multiple input runs, if required
        inputWorkspace = self._sumWorkspaces(inputWorkspaces, False)
        firstTransWorkspace = self._sumWorkspaces(firstTransWorkspaces, True)
        secondTransWorkspace = self._sumWorkspaces(secondTransWorkspaces, True)
        # Check if we will need to sum banks as part of the reduction
        self._should_sum_banks(inputWorkspace, firstTransWorkspace, secondTransWorkspace)
        # Slice the input workspace, if required
        inputWorkspace = self._sliceWorkspace(inputWorkspace)
        # Perform the reduction
        alg = self._reduce(inputWorkspace, firstTransWorkspace, secondTransWorkspace)
        # Set outputs and tidy TOF workspaces into a group
        self._finalize(alg)
        self._groupTOFWorkspaces(inputWorkspaces)

    def _groupTOFWorkspaces(self, inputWorkspaces):
        """Put all of the TOF workspaces into a group called 'TOF' to hide some noise
        for the user."""
        if not self.getProperty(Prop.GROUP_TOF).value:
            return
        tofWorkspaces = set(inputWorkspaces)
        # If slicing, also group the monitor workspace (note that there is only one
        # input run when slicing)
        if self._slicingEnabled():
            tofWorkspaces.add(_monitorWorkspace(inputWorkspaces[0]))
        # Add summed segment workspaces to TOF group
        for workspace_name in inputWorkspaces:
            summed_seg_name = _summedSegmentWorkspace(workspace_name)
            if AnalysisDataService.doesExist(summed_seg_name):
                tofWorkspaces.add(summed_seg_name)
        # Create the group
        if self.getProperty(Prop.HIDE_INPUT).value:
            self._group_workspaces(tofWorkspaces, "__TOF")
        else:
            self._group_workspaces(tofWorkspaces, "TOF")

    def validateInputs(self):
        """Return a dictionary containing issues found in properties."""
        issues = dict()
        if len(self.getProperty(Prop.RUNS).value) > 1 and self.getProperty(Prop.SLICE).value:
            issues[Prop.SLICE] = "Cannot perform slicing when summing multiple input runs"
        return issues

    def _copy_properties_from_reduction_algorithm(self, properties):
        self.copyProperties("ReflectometryReductionOneAuto", properties)
        self._reduction_properties += properties

    def _declareRunProperties(self):
        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        lenValidator = StringArrayLengthValidator()
        lenValidator.setLengthMin(1)
        mandatoryInputRuns.add(lenValidator)
        # Add property for the input runs
        self.declareProperty(
            StringArrayProperty(Prop.RUNS, values=[], validator=mandatoryInputRuns),
            doc="A list of run numbers or workspace names for the input runs. Multiple runs will be summed before reduction.",
        )
        # Add properties from child algorithm
        properties = [
            "ThetaIn",
            "ThetaLogName",
        ]
        self._copy_properties_from_reduction_algorithm(properties)
        # Add properties for settings to apply to input runs
        self.declareProperty(Prop.RELOAD, True, doc="If true, reload input workspaces if they are of the incorrect type")
        self.declareProperty(Prop.GROUP_TOF, True, doc="If true, group the TOF workspaces")

        self.declareProperty(Prop.HIDE_INPUT, False, doc="If true, make the input workspaces invisible in the ADS.")

    def _declarePreprocessProperties(self):
        """Copy properties from the child preprocess algorithm"""
        properties = ["CalibrationFile"]
        self.copyProperties("ReflectometryISISPreprocess", properties)

    def _declareSumBanksProperties(self):
        """Copy properties from the child sum banks algorithm"""
        properties = ["ROIDetectorIDs"]
        self._copy_properties_from_reduction_algorithm(properties)

    def _declareSlicingProperties(self):
        """Copy properties from the child slicing algorithm and add our own custom ones"""
        self.declareProperty(Prop.SLICE, False, doc="If true, slice the input workspace")
        self.setPropertyGroup(Prop.SLICE, "Slicing")
        # Convenience variables for conditional properties
        whenSliceEnabled = EnabledWhenProperty(Prop.SLICE, PropertyCriterion.IsEqualTo, "1")

        self._slice_properties = ["TimeInterval", "LogName", "LogValueInterval", "UseNewFilterAlgorithm"]
        self.copyProperties("ReflectometrySliceEventWorkspace", self._slice_properties)
        for property in self._slice_properties:
            self.setPropertySettings(property, whenSliceEnabled)
            self.setPropertyGroup(property, "Slicing")

        self.declareProperty(
            name=Prop.NUMBER_OF_SLICES,
            defaultValue=Property.EMPTY_INT,
            validator=IntBoundedValidator(lower=1),
            direction=Direction.Input,
            doc="The number of uniform-length slices to slice the input workspace into",
        )
        self.setPropertySettings(Prop.NUMBER_OF_SLICES, whenSliceEnabled)
        self.setPropertyGroup(Prop.NUMBER_OF_SLICES, "Slicing")

    def _declareReductionProperties(self):
        properties = [
            "SummationType",
            "ReductionType",
            "IncludePartialBins",
            "AnalysisMode",
            "ProcessingInstructions",
            "CorrectDetectors",
            "DetectorCorrectionType",
            "WavelengthMin",
            "WavelengthMax",
            "I0MonitorIndex",
            "MonitorBackgroundWavelengthMin",
            "MonitorBackgroundWavelengthMax",
            "MonitorIntegrationWavelengthMin",
            "MonitorIntegrationWavelengthMax",
            "SubtractBackground",
            "BackgroundProcessingInstructions",
            "BackgroundCalculationMethod",
            "DegreeOfPolynomial",
            "CostFunction",
            "NormalizeByIntegratedMonitors",
            "CorrectionAlgorithm",
            "Polynomial",
            "C0",
            "C1",
        ]
        self._copy_properties_from_reduction_algorithm(properties)

    def _declareTransmissionProperties(self):
        # Add input transmission run properties
        self.declareProperty(
            StringArrayProperty(Prop.FIRST_TRANS_RUNS, values=[]),
            doc="A list of run numbers or workspace names for the first transmission run. Multiple runs will be summed before reduction.",
        )
        self.setPropertyGroup(Prop.FIRST_TRANS_RUNS, "Transmission")
        self.declareProperty(
            StringArrayProperty(Prop.SECOND_TRANS_RUNS, values=[]),
            doc="A list of run numbers or workspace names for the second transmission run. Multiple runs will be summed before reduction.",
        )
        self.setPropertyGroup(Prop.SECOND_TRANS_RUNS, "Transmission")
        # Add properties copied from child algorithm
        properties = ["Params", "StartOverlap", "EndOverlap", "ScaleRHSWorkspace", "TransmissionProcessingInstructions"]
        self._copy_properties_from_reduction_algorithm(properties)

    def _declareOutputProperties(self):
        properties = [
            Prop.DEBUG,
            "MomentumTransferMin",
            "MomentumTransferStep",
            "MomentumTransferMax",
            "ScaleFactor",
            Prop.OUTPUT_WS_BINNED,
            Prop.OUTPUT_WS,
            Prop.OUTPUT_WS_LAM,
            Prop.OUTPUT_WS_TRANS,
            Prop.OUTPUT_WS_FIRST_TRANS,
            Prop.OUTPUT_WS_SECOND_TRANS,
        ]
        self._copy_properties_from_reduction_algorithm(properties)

    def _declareFloodCorrectionProperties(self):
        self._copy_properties_from_reduction_algorithm(["FloodCorrection"])
        self.declareProperty(
            "FloodWorkspace",
            "",
            "A flood workspace or filename to apply. If empty and FloodCorrection is 'Workspace' then no correction is applied.",
            Direction.Input,
        )

    def _declarePolarizationEfficiencyProperties(self):
        self._copy_properties_from_reduction_algorithm(["PolarizationAnalysis"])
        self.declareProperty(
            "PolarizationEfficiencies",
            "",
            "A workspace or file name containing the polarization efficiency factors for either "
            "the Wildes or Fredrikze correction methods.",
            Direction.Input,
        )
        self.declareProperty(
            "FredrikzePolarizationEfficienciesSpinStateOrder",
            "",
            "The spin state order of the workspaces in the workspace group to be passed to "
            'PolarizationCorrectionsFredrikze. See the "Spin State Configurations" -> '
            '"InputSpinStates" section of the PolarizationCorrectionsFredrikze v1 documentation for '
            "more details. This is only applied to Fredrikze corrections. Wildes flipper "
            "configurations are taken from the instrument's parameter file.",
            Direction.Input,
        )

    def _getInputWorkspaces(self, runs, isTrans):
        """Convert the given run numbers into real workspace names. Uses workspaces from
        the ADS if they exist, or loads them otherwise."""
        workspaces = list()
        for run in runs:
            ws = self._getRunFromADSOrNone(run, isTrans)
            if not ws:
                ws = self._loadRun(run, isTrans)
            if not ws:
                raise RuntimeError("Error loading run " + run)
            workspaces.append(ws)
        return workspaces

    def _loadFloodCorrectionWorkspace(self):
        flood_workspace = self.getPropertyValue("FloodWorkspace")
        if not flood_workspace:
            return None

        if AnalysisDataService.doesExist(flood_workspace):
            self.log().information(f'Loading flood correction information from workspace "{flood_workspace}"')
            return AnalysisDataService.retrieve(flood_workspace)

        alg = self.createChildAlgorithm("LoadNexus")
        try:
            alg.setRethrows(True)
            alg.setProperty("Filename", flood_workspace)
            alg.execute()
        except:
            raise RuntimeError(f'Could not load flood correction information from file "{flood_workspace}"')

        ws = alg.getProperty("OutputWorkspace").value
        self.log().information(f'Loaded flood correction information from file "{flood_workspace}"')
        return ws

    def _loadPolarizationCorrectionWorkspace(self):
        efficiencies_ws = self.getPropertyValue("PolarizationEfficiencies")
        if not efficiencies_ws:
            return None

        if AnalysisDataService.doesExist(efficiencies_ws):
            self.log().information(f'Loading polarization efficiency information from workspace "{efficiencies_ws}"')
            return AnalysisDataService.retrieve(efficiencies_ws)

        alg = self.createChildAlgorithm("LoadNexus")
        try:
            alg.setRethrows(True)
            alg.setProperty("Filename", efficiencies_ws)
            alg.execute()
        except:
            raise RuntimeError(f'Could not load polarization efficiency information from file "{efficiencies_ws}"')

        ws = alg.getProperty("OutputWorkspace").value
        self.log().information(f'Loaded polarization efficiency information from file "{efficiencies_ws}"')
        return ws

    def _should_sum_banks(self, inputWorkspace, firstTransWorkspace, secondTransWorkspace):
        """Checks if we should perform a sum banks step as part of the reduction and sets the ROIDetectorIDs property accordingly"""
        if self.getProperty(Prop.ROI_DETECTOR_IDS).isDefault:
            # Only sum banks when a region of detector IDs to sum has been passed in
            return

        perform_sum = []
        for workspace_name in [inputWorkspace, firstTransWorkspace, secondTransWorkspace]:
            if workspace_name is not None:
                workspace = AnalysisDataService.retrieve(workspace_name)
                perform_sum.append(self._has_single_2D_rectangular_detector(workspace))

        should_sum = perform_sum[0]
        if not all(sum_ws == should_sum for sum_ws in perform_sum):
            raise RuntimeError("Not implemented when some but not all input and transmission workspaces require summing across banks")

        if not should_sum:
            # Setting the property to its default will prevent summing taking place
            self.setProperty(Prop.ROI_DETECTOR_IDS, "")

    def _has_single_2D_rectangular_detector(self, workspace) -> bool:
        """Returns true if workspace has a single 2D rectangular detector, and is not a group workspace."""
        is_group = isinstance(workspace, WorkspaceGroup)
        if is_group:
            workspace = workspace[0]

        rect_detectors = workspace.getInstrument().findRectDetectors()
        num_rect_detectors = len(rect_detectors)

        if num_rect_detectors == 0:
            return False

        if num_rect_detectors != 1:
            raise NotImplementedError(f"Not implemented for more than one rectangular detector, {num_rect_detectors} were found.")

        # We don't sum banks for a linear detector
        if rect_detectors[0].xpixels() == 1 or rect_detectors[0].ypixels() == 1:
            return False

        if is_group:
            raise NotImplementedError("Not implemented for a WorkspaceGroup containing 2D detectors.")

        if not self._all_spectra_refer_to_rectangular_detector(workspace, rect_detectors[0]):
            return False

        return True

    @staticmethod
    def _all_spectra_refer_to_rectangular_detector(workspace, rectangular_detector) -> bool:
        """Checks if all data in a workspace is from the rectangular detector."""
        rect_det_id_start = rectangular_detector.minDetectorID()
        rect_det_id_end = rectangular_detector.maxDetectorID()
        ws_has_detectors = False

        for ws_index in range(workspace.getNumberHistograms()):
            try:
                det = workspace.getDetector(ws_index)
                if not det.isMonitor():
                    det_id = det.getID()
                    if not rect_det_id_start <= det_id <= rect_det_id_end:
                        # Workspace contains data that is not from the rectangular detector
                        return False
                    ws_has_detectors = True
            except RuntimeError:
                # Ignore detectors that don't have IDs
                continue

        return ws_has_detectors

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
            message = "Workspace " + workspace_name + " exists but is not a " + workspace_id
            if self._reload:
                self.log().information(message)
                _removeWorkspace(workspace_name)
                return False
            else:
                raise RuntimeError(message)

        # For event workspaces, the monitors workspace must also exist, otherwise it's not valid
        if workspace_id == "EventWorkspace":
            if not AnalysisDataService.doesExist(_monitorWorkspace(workspace_name)):
                message = "Monitors workspace " + workspace_name + "_monitors does not exist"
                if self._reload:
                    self.log().information(message)
                    _removeWorkspace(workspace_name)
                    return False
                else:
                    raise RuntimeError(message)
        return True

    def _workspaceExistsAndIsValid(self, workspace_name, isTrans):
        """Return true, if the given workspace exists in the ADS and is valid"""
        if not AnalysisDataService.doesExist(workspace_name):
            self.log().information("Workspace " + workspace_name + " does not exist")
            return False
        self.log().information("Workspace " + workspace_name + " exists")
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

    def _collapse_workspace_groups(self, workspaces):
        """Given a list of workspaces, which themselves could be groups of workspaces,
        return a new list of workspaces which are TOF"""
        ungrouped_workspaces = set([])
        delete_ws_group_flag = True
        for ws_name in workspaces:
            ws = AnalysisDataService.retrieve(ws_name)
            if isinstance(ws, WorkspaceGroup):
                ungrouped_workspaces = ungrouped_workspaces.union(self._collapse_workspace_groups(ws.getNames()))
                if delete_ws_group_flag is True:
                    AnalysisDataService.remove(ws_name)
            else:
                if (ws.getAxis(0).getUnit().unitID()) == "TOF":
                    ungrouped_workspaces.add(ws_name)
                else:
                    # Do not remove the workspace group from the ADS if a non-TOF workspace exists
                    delete_ws_group_flag = False
        return ungrouped_workspaces

    def _group_workspaces(self, workspaces, output_ws_name):
        """
        Groups all the given workspaces into a group with the given name. If the group
        already exists it will add them to that group.
        """
        if len(workspaces) < 1:
            return

        workspaces = self._collapse_workspace_groups(workspaces)

        if not workspaces:
            return

        if AnalysisDataService.doesExist(output_ws_name):
            ws_group = AnalysisDataService.retrieve(output_ws_name)
            if not isinstance(ws_group, WorkspaceGroup):
                raise RuntimeError("Cannot group TOF workspaces, a workspace called TOF already exists")
            else:
                for ws in workspaces:
                    if ws not in ws_group:
                        ws_group.add(ws)
        else:
            alg = self.createChildAlgorithm("GroupWorkspaces")
            alg.setProperty("InputWorkspaces", list(workspaces))
            alg.setProperty("OutputWorkspace", output_ws_name)
            alg.execute()
            ws_group = alg.getProperty("OutputWorkspace").value
            # We can't add the group as an output property or it will duplicate
            # the history for the contained workspaces, so add it directly to
            # the ADS
            AnalysisDataService.addOrReplace(output_ws_name, ws_group)

    def _loadRun(self, run, isTrans):
        """Load a run as an event workspace if slicing is requested, or a histogram
        workspace otherwise. Transmission runs are always loaded as histogram workspaces."""
        event_mode = not isTrans and self._slicingEnabled()
        args = {"InputRunList": [run], "EventMode": event_mode}
        calibration_filepath = self.getPropertyValue("CalibrationFile")
        if calibration_filepath:
            args["CalibrationFile"] = calibration_filepath
        alg = self.createChildAlgorithm("ReflectometryISISPreprocess", **args)
        alg.setRethrows(True)
        alg.execute()

        ws = alg.getProperty("OutputWorkspace").value
        monitor_ws = alg.getProperty("MonitorWorkspace").value
        workspace_name = self._prefixedName(_getRunNumberAsString(ws), isTrans)
        AnalysisDataService.addOrReplace(workspace_name, ws)
        if monitor_ws:
            AnalysisDataService.addOrReplace(_monitorWorkspace(workspace_name), monitor_ws)

        if event_mode:
            _throwIfNotValidReflectometryEventWorkspace(workspace_name)
            self.log().information("Loaded event workspace " + workspace_name)
        else:
            self.log().information("Loaded workspace " + workspace_name)
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
        summed_name = self._prefixedName(concatenated_names, isTrans)
        self.log().information("Summing workspaces" + " ".join(workspaces) + " into " + summed_name)
        summed_ws = MergeRuns(InputWorkspaces=", ".join(workspaces), OutputWorkspace=summed_name)
        # The reduction algorithm sets the output workspace names from the run number,
        # which by default is just the first run. Set it to the concatenated name,
        # e.g. 13461+13462
        if isinstance(summed_ws, WorkspaceGroup):
            for workspaceName in summed_ws.getNames():
                grouped_ws = AnalysisDataService.retrieve(workspaceName)
                grouped_ws.run().addProperty("run_number", concatenated_names, True)
        else:
            summed_ws.run().addProperty("run_number", concatenated_names, True)
        return summed_name

    def _slicingEnabled(self):
        return self.getProperty(Prop.SLICE).value

    def _setUniformNumberOfSlices(self, alg, workspace_name):
        """If slicing by a specified number of slices is requested, find the time
        interval to use to give this number of even time slices and set the relevant
        property on the given slicing algorithm"""
        if self.getProperty(Prop.NUMBER_OF_SLICES).isDefault:
            return
        number_of_slices = self.getProperty(Prop.NUMBER_OF_SLICES).value
        run = AnalysisDataService.retrieve(workspace_name).run()
        total_duration = (run.endTime() - run.startTime()).total_seconds()
        slice_duration = total_duration / number_of_slices
        alg.setProperty("TimeInterval", slice_duration)
        self.log().information(
            "Slicing " + workspace_name + " into " + str(number_of_slices) + " even slices of duration " + str(slice_duration)
        )

    def _setSliceStartStopTimes(self, alg, workspace_name):
        """Set the start/stop time for the slicing algorithm based on the
        run start/end times if the time interval is specified, otherwise
        we can end up with more slices than we expect"""
        if alg.getProperty("TimeInterval").isDefault:
            return
        run = AnalysisDataService.retrieve(workspace_name).run()
        alg.setProperty("StartTime", str(run.startTime()))
        alg.setProperty("StopTime", str(run.endTime()))

    def _runSliceAlgorithm(self, input_workspace, output_workspace):
        """Run the child algorithm to perform the slicing"""
        self.log().information("Running ReflectometrySliceEventWorkspace")
        alg = self.createChildAlgorithm("ReflectometrySliceEventWorkspace")
        for property in self._slice_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        alg.setProperty("OutputWorkspace", output_workspace)
        alg.setProperty("InputWorkspace", input_workspace)
        alg.setProperty("MonitorWorkspace", _monitorWorkspace(input_workspace))
        self._setUniformNumberOfSlices(alg, input_workspace)
        self._setSliceStartStopTimes(alg, input_workspace)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    def _sliceWorkspace(self, workspace):
        """If slicing has been requested, slice the input workspace, otherwise
        return it unchanged"""
        if not self._slicingEnabled():
            return workspace
        # Perform the slicing
        sliced_workspace_name = self._getSlicedWorkspaceGroupName(workspace)
        self.log().information("Slicing workspace " + workspace + " into " + sliced_workspace_name)
        workspace = self._runSliceAlgorithm(workspace, sliced_workspace_name)
        return sliced_workspace_name

    def _getSlicedWorkspaceGroupName(self, workspace):
        return workspace + "_sliced"

    def _reduce(self, input_workspace, first_trans_workspace, second_trans_workspace):
        """Run the child algorithm to do the reduction. Return the child algorithm."""
        self.log().information("Running ReflectometryReductionOneAuto on " + input_workspace)
        alg = self.createChildAlgorithm("ReflectometryReductionOneAuto")
        # Set properties that we copied directly from the child
        for property in self._reduction_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        # Set properties that we could not take directly from the child
        alg.setProperty("InputWorkspace", input_workspace)
        alg.setProperty("FirstTransmissionRun", first_trans_workspace)
        alg.setProperty("SecondTransmissionRun", second_trans_workspace)
        flood_workspace = self._loadFloodCorrectionWorkspace()
        if flood_workspace:
            alg.setProperty("FloodWorkspace", flood_workspace)
        alg.setProperty("HideSummedWorkspaces", self.getProperty(Prop.HIDE_INPUT).value)
        efficiencies_ws = self._loadPolarizationCorrectionWorkspace()
        if efficiencies_ws:
            alg.setProperty("PolarizationEfficiencies", efficiencies_ws)
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

    def _hasTransmissionRuns(self):
        return not self.getProperty(Prop.FIRST_TRANS_RUNS).isDefault

    def _isDebug(self):
        return not self.getProperty(Prop.DEBUG).isDefault

    def _finalize(self, child_alg):
        """Set our output properties from the results in the given child algorithm"""
        # Set the main workspace outputs
        self._setOutputProperty(Prop.OUTPUT_WS, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_BINNED, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_LAM, child_alg)
        # Set the Q params as outputs if they were not specified as inputs
        self._setOutputPropertyIfInputNotSet(Prop.QMIN, child_alg)
        self._setOutputPropertyIfInputNotSet(Prop.QSTEP, child_alg)
        self._setOutputPropertyIfInputNotSet(Prop.QMAX, child_alg)
        # Set the transmission workspace outputs
        self._setOutputProperty(Prop.OUTPUT_WS_TRANS, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_FIRST_TRANS, child_alg)
        self._setOutputProperty(Prop.OUTPUT_WS_SECOND_TRANS, child_alg)

    def _setOutputProperty(self, property_name, child_alg):
        """Set the given output property from the result in the given child algorithm,
        if it exists in the child algorithm's outputs"""
        value_name = child_alg.getPropertyValue(property_name)
        if value_name:
            self.setPropertyValue(property_name, value_name)
            value = child_alg.getProperty(property_name).value
            if value:
                self.setProperty(property_name, value)

    def _setOutputPropertyIfInputNotSet(self, property_name, child_alg):
        """Set the given output property from the result in the given child algorithm,
        if it was not set as an input to this algorithm and if it exists in the
        child algorithm's outputs"""
        if self.getProperty(property_name).isDefault:
            self._setOutputProperty(property_name, child_alg)


def _throwIfNotValidReflectometryEventWorkspace(workspace_name):
    workspace = AnalysisDataService.retrieve(workspace_name)
    if isinstance(workspace, WorkspaceGroup):
        raise RuntimeError("Slicing workspace groups is not supported")
    if not workspace.run().hasProperty("proton_charge"):
        raise RuntimeError("Cannot slice workspace: run must contain proton_charge")


def _monitorWorkspace(workspace):
    """Return the associated monitor workspace name for the given workspace"""
    return workspace + "_monitors"


def _summedSegmentWorkspace(workspace):
    """Return the associated summed segment workspace name for the given workspace"""
    return workspace + "_summed_segment"


def _getRunNumberAsString(workspace):
    """Get the run number for a workspace. If it's a workspace group, get
    the run number from the first child workspace."""
    try:
        if not isinstance(workspace, WorkspaceGroup):
            return str(workspace.getRunNumber())
        # Get first child in the group
        return str(workspace[0].getRunNumber())
    except:
        raise RuntimeError("Could not find run number for workspace " + workspace.getName())


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
    if AnalysisDataService.doesExist(_summedSegmentWorkspace(workspace_name)):
        _removeWorkspace(_summedSegmentWorkspace(workspace_name))


AlgorithmFactory.subscribe(ReflectometryISISLoadAndProcess)
