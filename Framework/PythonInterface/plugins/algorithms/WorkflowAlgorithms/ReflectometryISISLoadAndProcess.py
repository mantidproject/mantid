# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, AnalysisDataService, DataProcessorAlgorithm, WorkspaceGroup

from contextlib import contextmanager
from uuid import uuid4

from mantid.simpleapi import RenameWorkspace

from mantid.kernel import (
    CompositeValidator,
    Direction,
    EnabledWhenProperty,
    FloatBoundedValidator,
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
    DIAGNOSTICS = "Diagnostics"
    HIDE_INPUT = "HideInputWorkspaces"
    OUTPUT_WS = "OutputWorkspace"
    OUTPUT_WS_BINNED = "OutputWorkspaceBinned"
    OUTPUT_WS_LAM = "OutputWorkspaceWavelength"
    MULT_OUTPUT_WS = "OutputWorkspaces"
    MULT_OUTPUT_WS_BINNED = "OutputWorkspacesBinned"
    MULT_OUTPUT_WS_LAM = "OutputWorkspacesWavelength"
    OUTPUT_WS_FIRST_TRANS = "OutputWorkspaceFirstTransmission"
    OUTPUT_WS_SECOND_TRANS = "OutputWorkspaceSecondTransmission"
    OUTPUT_WS_TRANS = "OutputWorkspaceTransmission"
    NOMINAL_SPECULAR_PIXEL = "NominalSpecularPixelSpectrumNo"
    ACTUAL_SPECULAR_PIXEL = "ActualSpecularPixelSpectrumNo"


class ReflectometryISISLoadAndProcess(DataProcessorAlgorithm):
    _CALIBRATION_FILE_LOG = "reflectometry_calibration_file"
    _ADJUSTED_THETA_LOG = "reflectometry_adjusted_theta"
    _PROCESSED_TOF_ID_LOG = "reflectometry_processed_tof_id"
    _CACHED_GROUP_NAME_LOG = "reflectometry_cached_group_name"
    _POLREF = "POLREF"
    _POLREF_START_WS_INDEX = 4
    _INPUT_WS_INDEX = 0
    _FIRST_TRANS_WS_INDEX = 1
    _SECOND_TRANS_WS_INDEX = 2

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
        self._declareCalibrationProperties()
        self._declareSumBanksProperties()
        self._declareSlicingProperties()
        self._declareReductionProperties()
        self._declareTransmissionProperties()
        self._declareOutputProperties()
        self._declareFloodCorrectionProperties()
        self._declarePolarizationEfficiencyProperties()

    def PyExec(self):
        """Execute the algorithm."""
        cloned_workspaces, input_workspace_names = self._load()
        cloned_workspace_names, cloned_workspace_objects = zip(*cloned_workspaces)
        adjusted_theta = self._calibrate(*cloned_workspace_objects)
        self._should_sum_banks(*cloned_workspace_objects)
        if self._slicingEnabled():
            # clones monitor workspace - input detector workspace is already a clone
            to_reduce = self._sliceWorkspace(cloned_workspace_objects[self._INPUT_WS_INDEX], cloned_workspace_names[self._INPUT_WS_INDEX])
        else:
            to_reduce = [cloned_workspace_objects[self._INPUT_WS_INDEX]]
        # Perform the reduction
        is_multi_reduction = len(to_reduce) > 1
        finished_algs = []
        for cloned_workspace_slice in to_reduce:
            # replace original input workspace with the slice
            slice_specific_input_workspaces = [
                (cloned_workspace_names[self._INPUT_WS_INDEX], cloned_workspace_slice),
                *cloned_workspaces[self._FIRST_TRANS_WS_INDEX :],
            ]
            alg = self._reduce(
                slice_specific_input_workspaces,
                adjusted_theta,
                is_multi_reduction,
            )
            finished_algs.append(alg)
        # Set outputs and tidy TOF workspaces into a group
        self._finalize(finished_algs)
        processed_workspace_name = self._publish_processed_input(cloned_workspace_objects[self._INPUT_WS_INDEX], finished_algs)
        self._groupTOFWorkspaces(input_workspace_names)
        self._groupProcessedTOFWorkspaces([processed_workspace_name, *(workspace.name() for workspace in to_reduce)], input_workspace_names)

    def _publish_processed_input(self, cloned_workspace, finished_algs):
        """Retain the full-detector working input for diagnostics without another copy."""
        workspace_name = finished_algs[0].getPropertyValue(Prop.OUTPUT_WS_BINNED) + "_processed_tof"
        if self.getProperty(Prop.HIDE_INPUT).value and not workspace_name.startswith("__"):
            workspace_name = "__" + workspace_name

        def publish(workspace, name):
            if isinstance(workspace, WorkspaceGroup):
                for index, member in enumerate(workspace):
                    publish(member, f"{name}_{index + 1}")
            else:
                workspace.mutableRun().addProperty(self._PROCESSED_TOF_ID_LOG, workspace_name, True)
            AnalysisDataService.addOrReplace(name, workspace)

        publish(cloned_workspace, workspace_name)
        for alg in finished_algs:
            for property_name in (Prop.OUTPUT_WS, Prop.OUTPUT_WS_BINNED, Prop.OUTPUT_WS_LAM):
                output_workspace = alg.getProperty(property_name).value
                for member in output_workspace if isinstance(output_workspace, WorkspaceGroup) else [output_workspace]:
                    if member is not None:
                        member.mutableRun().addProperty(self._PROCESSED_TOF_ID_LOG, workspace_name, True)
        return workspace_name

    def _load(self):
        hide_inputs = self.getProperty(Prop.HIDE_INPUT).value
        self._tofPrefix = "__TOF_" if hide_inputs else "TOF_"
        self._transPrefix = "__TRANS_" if hide_inputs else "TRANS_"
        self._reload = self.getProperty(Prop.RELOAD).value
        # Convert run numbers to real workspaces
        inputRuns = self.getProperty(Prop.RUNS).value
        input_workspace_names = self._getInputWorkspaces(inputRuns, False)
        firstTransRuns = self.getProperty(Prop.FIRST_TRANS_RUNS).value
        first_transmission_workspace_names = self._getInputWorkspaces(firstTransRuns, True)
        secondTransRuns = self.getProperty(Prop.SECOND_TRANS_RUNS).value
        second_transmission_workspace_names = self._getInputWorkspaces(secondTransRuns, True)
        # clones input, either directly (single inputs) or via merge.
        cloned_input_workspace, cloned_input_workspace_name = self._sumWorkspaces(input_workspace_names, False)
        cloned_first_transmission_workspace, cloned_first_transmission_workspace_name = self._sumWorkspaces(
            first_transmission_workspace_names, True
        )
        cloned_second_transmission_workspace, cloned_second_transmission_workspace_name = self._sumWorkspaces(
            second_transmission_workspace_names, True
        )
        reduction_inputs = (
            (cloned_input_workspace_name, cloned_input_workspace),
            (cloned_first_transmission_workspace_name, cloned_first_transmission_workspace),
            (cloned_second_transmission_workspace_name, cloned_second_transmission_workspace),
        )
        return reduction_inputs, input_workspace_names

    def _calibrate(self, cloned_workspace, cloned_first_transmission_workspace, cloned_second_transmission_workspace):
        nominal_specular_pixel = self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).value
        if self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).isDefault:
            nominal_specular_pixel = self.getProperty(Prop.ACTUAL_SPECULAR_PIXEL).value
        cloned_workspace, adjusted_theta = self._calibrate_workspace(cloned_workspace, adjust_theta=True)
        cloned_first_transmission_workspace, _ = self._calibrate_workspace(
            cloned_first_transmission_workspace, experiment_angle=0, specular_spectrum_no=nominal_specular_pixel
        )
        cloned_second_transmission_workspace, _ = self._calibrate_workspace(
            cloned_second_transmission_workspace, experiment_angle=0, specular_spectrum_no=nominal_specular_pixel
        )
        return adjusted_theta

    def _groupTOFWorkspaces(self, inputWorkspaces):
        """Group cached inputs and monitors in TOF or __TOF."""
        if not self.getProperty(Prop.GROUP_TOF).value:
            return
        tofWorkspaces = list(inputWorkspaces)
        # If slicing, also group the monitor workspace (note that there is only one
        # input run when slicing)
        if self._slicingEnabled():
            tofWorkspaces.append(_monitorWorkspace(inputWorkspaces[0]))
        # Create the group
        if self.getProperty(Prop.HIDE_INPUT).value:
            self._group_workspaces(tofWorkspaces, "__TOF")
        else:
            self._group_workspaces(tofWorkspaces, "TOF")

        # Remove group workspaces whose members have now been flattened into TOF
        cached_group_names = [
            workspace_name
            for workspace_name in dict.fromkeys(tofWorkspaces)
            if AnalysisDataService.doesExist(workspace_name) and isinstance(AnalysisDataService.retrieve(workspace_name), WorkspaceGroup)
        ]
        for workspace_name in cached_group_names:
            AnalysisDataService.remove(workspace_name)

    def _groupProcessedTOFWorkspaces(self, workspace_names, input_workspace_names):
        """Group processed inputs, collapsed slices and bank sums in TOF_processed or __TOF_processed."""
        if not self.getProperty(Prop.GROUP_TOF).value:
            return
        processed_workspace_names = list(workspace_names)
        for input_workspace_name in input_workspace_names:
            summed_workspace_name = _summedSegmentWorkspace(input_workspace_name)
            if AnalysisDataService.doesExist(summed_workspace_name):
                processed_workspace_names.append(summed_workspace_name)
        processed_workspace_names = list(dict.fromkeys(processed_workspace_names))
        group_name = "__TOF_processed" if self.getProperty(Prop.HIDE_INPUT).value else "TOF_processed"
        self._group_workspaces(processed_workspace_names, group_name)
        # Only the flattened TOF members are needed after successful grouping.
        for workspace_name in processed_workspace_names:
            if isinstance(AnalysisDataService.retrieve(workspace_name), WorkspaceGroup):
                AnalysisDataService.remove(workspace_name)

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

    def _declareCalibrationProperties(self):
        """Declare properties controlling detector calibration."""
        self.copyProperties("ReflectometryISISCalibration", ["CalibrationFile", Prop.NOMINAL_SPECULAR_PIXEL])
        self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).clearSettings()
        self.declareProperty(
            Prop.ACTUAL_SPECULAR_PIXEL,
            Property.EMPTY_DBL,
            FloatBoundedValidator(lower=0.0),
            "For POLREF calibration, the actual specular pixel spectrum number, which may be fractional. "
            "If provided, this replaces peak fitting. If the nominal spectrum number is omitted or equal to this value, "
            "the experiment angle is not corrected. Otherwise the calibration-map angular difference is used to correct it.",
        )

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

        self._slice_properties = ["TimeInterval", "LogName", "LogValueInterval", "FilterByLogValue"]
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
            Prop.DIAGNOSTICS,
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
        self.declareProperty(
            StringArrayProperty(Prop.MULT_OUTPUT_WS, values=[], direction=Direction.Output),
            doc="A list of output workspaces. Only used when slicing a group workspace.",
        )
        self.declareProperty(
            StringArrayProperty(Prop.MULT_OUTPUT_WS_BINNED, values=[], direction=Direction.Output),
            doc="A list of binned output workspaces. Only used when slicing a group workspace.",
        )
        self.declareProperty(
            StringArrayProperty(Prop.MULT_OUTPUT_WS_LAM, values=[], direction=Direction.Output),
            doc="A list of wavelength output workspaces. Only used when slicing a group workspace.",
        )

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
            "PolarizationCorrectionInputSpinStateOrder",
            "",
            "The order of spin states in the input workspace group. For Wildes corrections this is the flipper "
            "configuration, e.g. '00,01,10,11' or '0,1'. For Fredrikze corrections this is the input spin state "
            "order, e.g. 'pa,ap,pp,aa' or 'p,a'. If left empty, the correction default from the parameter file or "
            "child algorithm is used.",
            Direction.Input,
        )
        self.declareProperty(
            "FredrikzePolarizationSpinStateOrder",
            "",
            "Deprecated. Use PolarizationCorrectionInputSpinStateOrder instead.",
            Direction.Input,
        )

    def _getInputWorkspaces(self, runs, isTrans):
        """Resolve run numbers to logical input names, reusing cached workspaces or loading them."""
        workspaces = []
        for run in runs:
            ws = self._getRunFromADSOrNone(run, isTrans)
            if ws is None:
                ws = self._loadRun(run, isTrans)
            if not ws:
                raise RuntimeError("Error loading run " + run)

            # Stamp group workspace members so their original group name can be identified when ungrouped.
            for workspace_name in (ws, _monitorWorkspace(ws)):
                workspace = self._get_cached_workspace(workspace_name)
                if isinstance(workspace, WorkspaceGroup):
                    for member in workspace:
                        member.mutableRun().addProperty(self._CACHED_GROUP_NAME_LOG, workspace_name, True)
            workspaces.append(ws)
        return workspaces

    def _clone_workspace(self, input_workspace):
        if isinstance(input_workspace, WorkspaceGroup):
            return self._create_workspace_group_from_members(
                [self._clone_workspace(input_member_workspace) for input_member_workspace in input_workspace]
            )
        alg = self.createChildAlgorithm("CloneWorkspace", InputWorkspace=input_workspace)
        alg.execute()
        return alg.getProperty("OutputWorkspace").value

    @contextmanager
    def _temporary_ads_workspaces(self, workspace_inputs, require_names=False):
        """Register inputs together, removing only registrations created in this scope."""
        workspace_names = {}

        def register(cloned_workspace, input_workspace_name):
            if cloned_workspace.name() and AnalysisDataService.doesExist(cloned_workspace.name()):
                return cloned_workspace.name()
            if isinstance(cloned_workspace, WorkspaceGroup):
                register_args = map_cloned_group_member_to_input_name(cloned_workspace, input_workspace_name)
                for cloned_member_workspace, input_member_workspace_name in register_args:
                    register(cloned_member_workspace, input_member_workspace_name)
            cloned_workspace_name = "_refl_work_" + uuid4().hex + "_" + input_workspace_name
            AnalysisDataService.add(cloned_workspace_name, cloned_workspace)
            workspace_names[cloned_workspace_name] = input_workspace_name
            return cloned_workspace_name

        def map_cloned_group_member_to_input_name(cloned_workspace, input_workspace_name):
            cached_input_group = self._get_cached_workspace(input_workspace_name)
            member_to_name_list = []
            for index, cloned_member_workspace in enumerate(cloned_workspace):
                # Preserve cached member names; merged groups have no cached group, so generate names.
                input_member_workspace_name = (
                    cached_input_group[index].name()
                    if isinstance(cached_input_group, WorkspaceGroup)
                    else f"{input_workspace_name}_{index + 1}"
                )
                member_to_name_list.append((cloned_member_workspace, input_member_workspace_name))
            return member_to_name_list

        try:
            input_properties = {}
            for property_name, input_workspace_name, cloned_workspace in workspace_inputs:
                property_value = cloned_workspace
                if cloned_workspace is not None and (require_names or isinstance(cloned_workspace, WorkspaceGroup)):
                    property_value = register(cloned_workspace, input_workspace_name)
                input_properties[property_name] = property_value
            yield input_properties, workspace_names
        finally:
            for cloned_workspace_name in reversed(workspace_names):
                AnalysisDataService.remove(cloned_workspace_name)

    @staticmethod
    def _rename_summed_workspaces(workspace_names):
        for cloned_workspace_name, input_workspace_name in workspace_names.items():
            for hidden_prefix in ("", "__"):
                summed_workspace_name = _summedSegmentWorkspace(hidden_prefix + cloned_workspace_name)
                if AnalysisDataService.doesExist(summed_workspace_name):
                    public_workspace_name = (
                        input_workspace_name if input_workspace_name.startswith("__") else hidden_prefix + input_workspace_name
                    )
                    RenameWorkspace(InputWorkspace=summed_workspace_name, OutputWorkspace=_summedSegmentWorkspace(public_workspace_name))

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
        for workspace, inspect_all_group_members in [
            (inputWorkspace, True),
            (firstTransWorkspace, False),
            (secondTransWorkspace, False),
        ]:
            if workspace is not None:
                if isinstance(workspace, WorkspaceGroup):
                    workspaces = workspace if inspect_all_group_members else [workspace[0]]
                else:
                    workspaces = [workspace]
                perform_sum.extend(self._has_single_2D_rectangular_detector(member) for member in workspaces)

        should_sum = perform_sum[0]
        if not all(sum_ws == should_sum for sum_ws in perform_sum):
            raise RuntimeError("Not implemented when some but not all input and transmission workspaces require summing across banks")

        if not should_sum:
            # Setting the property to its default will prevent summing taking place
            self.setProperty(Prop.ROI_DETECTOR_IDS, "")

    def _has_single_2D_rectangular_detector(self, workspace) -> bool:
        """Returns true if workspace has a single 2D rectangular detector."""

        rect_detectors = workspace.getInstrument().findRectDetectors()
        num_rect_detectors = len(rect_detectors)

        if num_rect_detectors == 0:
            return False

        if num_rect_detectors != 1:
            raise NotImplementedError(f"Not implemented for more than one rectangular detector, {num_rect_detectors} were found.")

        # We don't sum banks for a linear detector
        component_info = workspace.componentInfo()
        bank_index = component_info.indexOfAny(rect_detectors[0].getName())
        if component_info.pixelGridNX(bank_index) == 1 or component_info.pixelGridNY(bank_index) == 1:
            return False

        if not self._all_spectra_refer_to_rectangular_detector(workspace, rect_detectors[0]):
            return False

        return True

    @staticmethod
    def _all_spectra_refer_to_rectangular_detector(workspace, rectangular_detector) -> bool:
        """Checks if all data in a workspace is from the rectangular detector."""
        component_info = workspace.componentInfo()
        bank_index = component_info.indexOfAny(rectangular_detector.getName())
        rect_det_id_start = component_info.pixelGridMinDetectorID(bank_index)
        rect_det_id_end = component_info.pixelGridMaxDetectorID(bank_index)
        ws_has_detectors = False

        spectrum_info = workspace.spectrumInfo()
        for ws_index in range(workspace.getNumberHistograms()):
            # Ignore monitors and spectra that don't have detectors
            if not spectrum_info.hasDetectors(ws_index) or spectrum_info.isMonitor(ws_index):
                continue
            for det_id in workspace.getSpectrum(ws_index).getDetectorIDs():
                if not rect_det_id_start <= det_id <= rect_det_id_end:
                    # Workspace contains data that is not from the rectangular detector
                    return False
            ws_has_detectors = True

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

        def check_for_monitors(ws_name):
            if not AnalysisDataService.doesExist(_monitorWorkspace(ws_name)):
                message = "Monitors workspace " + ws_name + "_monitors does not exist"
                if self._reload:
                    self.log().information(message)
                    _removeWorkspace(ws_name)
                    return False
                else:
                    raise RuntimeError(message)
            return True

        workspace = self._get_cached_workspace(workspace_name)

        if self._representative_workspace(workspace).id() != workspace_id:
            message = "Workspace " + workspace_name + " exists but is not a " + workspace_id
            if self._reload:
                self.log().information(message)
                if isinstance(workspace, WorkspaceGroup):
                    for member in list(workspace):
                        _removeWorkspace(member.name())
                _removeWorkspace(workspace_name)
                return False
            else:
                raise RuntimeError(message)

        # For event workspaces, the monitors workspace must also exist, otherwise it's not valid
        if workspace_id == "EventWorkspace":
            if workspace.isGroup():
                return all([check_for_monitors(member.name()) for member in workspace])
            return check_for_monitors(workspace_name)
        return True

    def _workspaceExistsAndIsValid(self, workspace_name, isTrans):
        """Return true, if the given workspace exists in the ADS and is valid"""
        if self._get_cached_workspace(workspace_name) is None:
            self.log().information("Workspace " + workspace_name + " does not exist")
            return False
        self.log().information("Workspace " + workspace_name + " exists")
        if not isTrans and self._slicingEnabled():
            is_valid = self._isValidWorkspace(workspace_name, "EventWorkspace")
        else:
            is_valid = self._isValidWorkspace(workspace_name, "Workspace2D")

        if not is_valid:
            return False

        return True

    @staticmethod
    def _representative_workspace(workspace):
        return workspace[0] if isinstance(workspace, WorkspaceGroup) else workspace

    def _get_cached_workspace(self, workspace_name):
        """Resolve an ADS input or its ordered members in a flattened TOF cache."""
        if AnalysisDataService.doesExist(workspace_name):
            return AnalysisDataService.retrieve(workspace_name)
        for group_name in ("TOF", "__TOF"):
            if not AnalysisDataService.doesExist(group_name):
                continue
            members = [
                member
                for member in AnalysisDataService.retrieve(group_name)
                if member.run().hasProperty(self._CACHED_GROUP_NAME_LOG)
                and member.run().getProperty(self._CACHED_GROUP_NAME_LOG).value == workspace_name
            ]
            if members:
                return self._create_workspace_group_from_members(members)
        return None

    def _getRunFromADSOrNone(self, run, isTrans):
        """Return the logical name of a valid cached input, with or without its prefix."""
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
        """Return unique TOF members in input order."""
        ungrouped_workspaces = []
        for name in workspaces:
            workspace = self._get_cached_workspace(name)
            if isinstance(workspace, WorkspaceGroup):
                ungrouped_workspaces.extend(self._collapse_workspace_groups(workspace.getNames()))
            elif workspace.getAxis(0).getUnit().unitID() == "TOF":
                ungrouped_workspaces.append(name)
        return list(dict.fromkeys(ungrouped_workspaces))

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
            alg.setProperty("InputWorkspaces", workspaces)
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
        args = self._preprocess_arguments(run, event_mode)
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

    def _preprocess_arguments(self, run, event_mode):
        args = {"InputRunList": [run], "EventMode": event_mode}
        return args

    def _calibrate_workspace(self, workspace, adjust_theta=False, experiment_angle=None, specular_spectrum_no=None):
        calibration_filepath = self.getPropertyValue("CalibrationFile")
        if workspace is None or not calibration_filepath:
            return workspace, experiment_angle
        representative_workspace = self._representative_workspace(workspace)
        self._validate_instrument_specific_calibration_properties(representative_workspace)

        is_group = isinstance(workspace, WorkspaceGroup)
        members = list(workspace) if is_group else [workspace]
        calibrated_members = []
        adjusted_theta = experiment_angle
        for member_index, member in enumerate(members):
            alg = self.createChildAlgorithm("ReflectometryISISCalibration")
            alg.setRethrows(True)
            alg.setProperty("InputWorkspace", member)
            alg.setProperty("PreserveInputWorkspace", False)
            alg.setProperty("CalibrationFile", calibration_filepath)
            experiment_angle, has_angle_dependent_geometry = self._setInstrumentSpecificProperties(
                alg, representative_workspace, adjust_theta, experiment_angle, specular_spectrum_no
            )
            alg.execute()
            calibrated_member = alg.getProperty("OutputWorkspace").value
            calibrated_member.mutableRun().addProperty(self._CALIBRATION_FILE_LOG, calibration_filepath, True)
            #  Only return the adjusted theta from the first member of a group, since it should be the same for all members
            if has_angle_dependent_geometry and member_index == 0:
                adjusted_theta = experiment_angle
                if adjust_theta and calibrated_member.run().hasProperty(self._ADJUSTED_THETA_LOG):
                    adjusted_theta = calibrated_member.run().getProperty(self._ADJUSTED_THETA_LOG).value
            calibrated_members.append(calibrated_member)

        calibrated_workspace = self._create_workspace_group_from_members(calibrated_members) if is_group else calibrated_members[0]
        return calibrated_workspace, adjusted_theta

    def _create_workspace_group_from_members(self, members):
        group = WorkspaceGroup()
        for member in members:
            group.addWorkspace(member)
        return group

    def _setInstrumentSpecificProperties(self, calibration_alg, workspace, adjust_theta, experiment_angle=None, specular_spectrum_no=None):
        if workspace.getInstrumentName() != self._POLREF:
            return experiment_angle, False

        nominal_specular_pixel = self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).value
        if self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).isDefault:
            nominal_specular_pixel = self.getProperty(Prop.ACTUAL_SPECULAR_PIXEL).value
        if specular_spectrum_no is None:
            actual_specular_pixel = self.getProperty(Prop.ACTUAL_SPECULAR_PIXEL)
            specular_spectrum_no = (
                self._find_specular_pixel_spectrum_no(workspace) if actual_specular_pixel.isDefault else actual_specular_pixel.value
            )
        if experiment_angle is None:
            experiment_angle = self._experiment_angle(workspace)

        calibration_alg.setProperty("InstrumentWorkflow", self._POLREF)
        calibration_alg.setProperty("SpecularPixelSpectrumNo", specular_spectrum_no)
        calibration_alg.setProperty("ExperimentAngle", experiment_angle)
        calibration_alg.setProperty("AdjustExperimentAngle", adjust_theta and specular_spectrum_no != nominal_specular_pixel)
        calibration_alg.setProperty(Prop.NOMINAL_SPECULAR_PIXEL, nominal_specular_pixel)
        return experiment_angle, True

    def _validate_instrument_specific_calibration_properties(self, workspace):
        if workspace.getInstrumentName() != self._POLREF:
            return
        if self.getProperty("CorrectDetectors").value:
            raise RuntimeError("CorrectDetectors must be False when CalibrationFile is provided for POLREF data")
        if self.getProperty(Prop.ACTUAL_SPECULAR_PIXEL).isDefault and self.getProperty(Prop.NOMINAL_SPECULAR_PIXEL).isDefault:
            raise RuntimeError("ActualSpecularPixelSpectrumNo or NominalSpecularPixelSpectrumNo must be provided for POLREF calibration")

    def _find_specular_pixel_spectrum_no(self, workspace):
        lines_alg = self.createChildAlgorithm("FindReflectometryLines")
        lines_alg.setProperty("InputWorkspace", workspace)
        lines_alg.setProperty("StartWorkspaceIndex", self._POLREF_START_WS_INDEX)
        lines_alg.execute()
        return self._spectrum_number_for_workspace_index(workspace, lines_alg.getProperty("LineCentre").value)

    def _experiment_angle(self, workspace):
        theta = self.getProperty("ThetaIn")
        if not theta.isDefault:
            return theta.value

        theta_log_name = self.getPropertyValue("ThetaLogName")
        if theta_log_name:
            theta_log = workspace.run().getProperty(theta_log_name)
            if hasattr(theta_log, "lastValue"):
                return theta_log.lastValue()
            return float(theta_log.value)

        raise RuntimeError("ThetaIn or ThetaLogName must be provided when calibrating POLREF data")

    @staticmethod
    def _spectrum_number_for_workspace_index(workspace, workspace_index):
        lower_index = int(workspace_index)
        fraction = workspace_index - lower_index
        lower_spectrum_no = workspace.getSpectrum(lower_index).getSpectrumNo()
        if fraction == 0:
            return float(lower_spectrum_no)
        upper_spectrum_no = workspace.getSpectrum(lower_index + 1).getSpectrumNo()
        return lower_spectrum_no + fraction * (upper_spectrum_no - lower_spectrum_no)

    def _sumWorkspaces(self, input_workspace_names, isTrans):
        if not input_workspace_names:
            return None, ""
        if len(input_workspace_names) == 1:
            return self._clone_workspace(self._get_cached_workspace(input_workspace_names[0])), input_workspace_names[0]
        run_numbers = "+".join(self._removePrefix(input_workspace_name, isTrans) for input_workspace_name in input_workspace_names)
        workspace_inputs = [(str(index), name, self._get_cached_workspace(name)) for index, name in enumerate(input_workspace_names)]
        with self._temporary_ads_workspaces(workspace_inputs, require_names=True) as (input_properties, _):
            alg = self.createChildAlgorithm("MergeRuns", InputWorkspaces=list(input_properties.values()))
            alg.execute()
            summed_workspace = alg.getProperty("OutputWorkspace").value
        for member in summed_workspace if isinstance(summed_workspace, WorkspaceGroup) else [summed_workspace]:
            member.mutableRun().addProperty("run_number", run_numbers, True)
        return summed_workspace, self._prefixedName(run_numbers, isTrans)

    def _slicingEnabled(self):
        return self.getProperty(Prop.SLICE).value

    def _setUniformNumberOfSlices(self, alg, workspace_name):
        """If slicing by a specified number of slices is requested, find the time
        interval to use to give this number of even time slices and set the relevant
        property on the given slicing algorithm"""
        if self.getProperty(Prop.NUMBER_OF_SLICES).isDefault:
            return
        number_of_slices = self.getProperty(Prop.NUMBER_OF_SLICES).value
        workspace = AnalysisDataService.retrieve(workspace_name)
        run = workspace[0].run() if workspace.isGroup() else workspace.run()

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
        workspace = AnalysisDataService.retrieve(workspace_name)
        run = workspace[0].run() if workspace.isGroup() else workspace.run()
        alg.setProperty("StartTime", str(run.startTime()))
        alg.setProperty("StopTime", str(run.endTime()))

    def _runSliceAlgorithm(self, input_workspace_name, monitor_workspace_name, output_workspace_name):
        """Run the child algorithm to perform the slicing"""
        self.log().information("Running ReflectometrySliceEventWorkspace")
        alg = self.createChildAlgorithm("ReflectometrySliceEventWorkspace")
        for property in self._slice_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        alg.setProperty("OutputWorkspaceName", output_workspace_name)
        alg.setProperty("InputWorkspaceName", input_workspace_name)
        alg.setProperty("MonitorWorkspaceName", monitor_workspace_name)
        self._setUniformNumberOfSlices(alg, input_workspace_name)
        self._setSliceStartStopTimes(alg, input_workspace_name)
        alg.execute()

        return alg.getProperty("OutputWorkspaceNames").value

    # Clone the cached monitors before slicing; the supplied sample is already a working copy.
    def _sliceWorkspace(self, cloned_workspace, input_workspace_name):
        input_monitor_workspace_name = _monitorWorkspace(input_workspace_name)
        cloned_monitor_workspace = self._clone_workspace(self._get_cached_workspace(input_monitor_workspace_name))
        workspace_inputs = [
            ("InputWorkspaceName", input_workspace_name, cloned_workspace),
            ("MonitorWorkspaceName", input_monitor_workspace_name, cloned_monitor_workspace),
        ]
        with self._temporary_ads_workspaces(workspace_inputs, require_names=True) as (input_properties, workspace_names):
            sliced_workspace_name = input_workspace_name + "_sliced"
            output_workspace_names = self._runSliceAlgorithm(
                input_properties["InputWorkspaceName"], input_properties["MonitorWorkspaceName"], sliced_workspace_name
            )
            sliced_workspace_names = output_workspace_names if isinstance(output_workspace_names, list) else [sliced_workspace_name]
            # Slices are public outputs; retain input workspace member names for grouped data.
            for group_workspace_name in sliced_workspace_names:
                for member_workspace in AnalysisDataService.retrieve(group_workspace_name):
                    public_workspace_name = member_workspace.name()
                    for cloned_workspace_name, input_workspace_name in sorted(
                        workspace_names.items(), key=lambda item: len(item[0]), reverse=True
                    ):
                        public_workspace_name = public_workspace_name.replace(cloned_workspace_name, input_workspace_name)
                    if public_workspace_name != member_workspace.name():
                        RenameWorkspace(InputWorkspace=member_workspace, OutputWorkspace=public_workspace_name)
        return [AnalysisDataService.retrieve(workspace_name) for workspace_name in sliced_workspace_names]

    def _reduce(
        self,
        cloned_workspaces,
        adjusted_theta=None,
        multi_reduction: bool = False,
    ):
        """Run the child algorithm to do the reduction. Return the child algorithm."""
        self.log().information("Running ReflectometryReductionOneAuto")
        alg = self.createChildAlgorithm("ReflectometryReductionOneAuto")
        # Set properties that we copied directly from the child
        for property in self._reduction_properties:
            alg.setProperty(property, self.getPropertyValue(property))
        if adjusted_theta is not None:
            alg.setProperty("ThetaIn", adjusted_theta)
        workspace_inputs = [(v, *t) for v, t in zip(("InputWorkspace", "FirstTransmissionRun", "SecondTransmissionRun"), cloned_workspaces)]
        with self._temporary_ads_workspaces(workspace_inputs) as (input_properties, workspace_names):
            for property_name, property_value in input_properties.items():
                alg.setProperty(property_name, property_value)
            alg.setProperty("PreserveInputWorkspace", False)
            flood_workspace = self._loadFloodCorrectionWorkspace()
            if flood_workspace:
                alg.setProperty("FloodWorkspace", flood_workspace)
            alg.setProperty("HideSummedWorkspaces", self.getProperty(Prop.HIDE_INPUT).value)
            efficiencies_ws = self._loadPolarizationCorrectionWorkspace()
            if efficiencies_ws:
                alg.setProperty("PolarizationEfficiencies", efficiencies_ws)
            if multi_reduction:
                workspace_name = cloned_workspaces[self._INPUT_WS_INDEX][1].name()
                alg.setProperty(Prop.OUTPUT_WS, f"IvsQ_{workspace_name}")
                alg.setProperty(Prop.OUTPUT_WS_BINNED, f"IvsQ_binned_{workspace_name}")
                alg.setProperty(Prop.OUTPUT_WS_LAM, f"IvsLam_{workspace_name}")
            alg.setProperty("PolarizationCorrectionInputSpinStateOrder", self._polarizationCorrectionInputSpinStateOrder())
            alg.execute()
            self._rename_summed_workspaces(workspace_names)
        return alg

    def _polarizationCorrectionInputSpinStateOrder(self):
        input_spin_state_order = self.getPropertyValue("PolarizationCorrectionInputSpinStateOrder")
        deprecated_input_spin_state_order = self.getPropertyValue("FredrikzePolarizationSpinStateOrder")
        if input_spin_state_order and deprecated_input_spin_state_order and input_spin_state_order != deprecated_input_spin_state_order:
            raise RuntimeError(
                "PolarizationCorrectionInputSpinStateOrder and FredrikzePolarizationSpinStateOrder cannot both be set to different values."
            )
        return input_spin_state_order or deprecated_input_spin_state_order

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

    def _isDiagnostics(self):
        return not self.getProperty(Prop.DIAGNOSTICS).isDefault

    def _finalize(self, finished_algs: list):
        """Set our output properties from the results in the given child algorithm"""
        # Set the main workspace outputs
        if not finished_algs:
            return
        if len(finished_algs) == 1:
            self.enableCheckForStringArrayWorkspaceOutputs(False)
            child_alg = finished_algs[0]
            self._setOutputProperty(Prop.OUTPUT_WS, child_alg)
            self._setOutputProperty(Prop.OUTPUT_WS_BINNED, child_alg)
            self._setOutputProperty(Prop.OUTPUT_WS_LAM, child_alg)
        else:
            self.enableCheckForStringArrayWorkspaceOutputs(True)
            outputs = []
            binned_outputs = []
            lambda_outputs = []
            for child_alg in finished_algs:
                outputs.append(child_alg.getPropertyValue(Prop.OUTPUT_WS))
                binned_outputs.append(child_alg.getPropertyValue(Prop.OUTPUT_WS_BINNED))
                lambda_outputs.append(child_alg.getPropertyValue(Prop.OUTPUT_WS_LAM))
            self.setProperty(Prop.MULT_OUTPUT_WS, outputs)
            self.setProperty(Prop.MULT_OUTPUT_WS_BINNED, binned_outputs)
            self.setProperty(Prop.MULT_OUTPUT_WS_LAM, lambda_outputs)

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
    def check_workspace(workspace):
        if isinstance(workspace, WorkspaceGroup):
            for member_ws in workspace:
                check_workspace(member_ws)
            return
        if not workspace.run().hasProperty("proton_charge"):
            raise RuntimeError("Cannot slice workspace: run must contain proton_charge")

    workspace = AnalysisDataService.retrieve(workspace_name)
    check_workspace(workspace)


def _monitorWorkspace(workspace_name):
    """Return the monitor workspace name associated with the input workspace name."""
    return workspace_name + "_monitors"


def _summedSegmentWorkspace(workspace_name):
    """Return the summed segment workspace name associated with the input workspace name."""
    return workspace_name + "_summed_segment"


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
