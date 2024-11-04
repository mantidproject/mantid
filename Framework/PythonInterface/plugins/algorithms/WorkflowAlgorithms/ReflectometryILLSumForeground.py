# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspace,
    MatrixWorkspaceProperty,
    PropertyMode,
    WorkspaceUnitValidator,
)
from mantid.kernel import (
    CompositeValidator,
    Direction,
    FloatArrayBoundedValidator,
    FloatArrayProperty,
    IntArrayBoundedValidator,
    IntArrayLengthValidator,
    IntArrayProperty,
    Property,
    StringListValidator,
)
from mantid.simpleapi import (
    CropWorkspace,
    Divide,
    ExtractSingleSpectrum,
    MoveInstrumentComponent,
    Plus,
    RebinToWorkspace,
    ReflectometryBeamStatistics,
    ReflectometrySumInQ,
    RotateInstrumentComponent,
)
import ReflectometryILL_common as common
import ILL_utilities as utils
import numpy
from typing import List


class Prop:
    CLEANUP = "Cleanup"
    DIRECT_WS = "DirectLineWorkspace"
    DIRECT_FOREGROUND_WS = "DirectForegroundWorkspace"
    FOREGROUND_INDICES = "Foreground"
    INPUT_WS = "InputWorkspace"
    OUTPUT_WS = "OutputWorkspace"
    SUBALG_LOGGING = "SubalgorithmLogging"
    SUM_TYPE = "SummationType"
    WAVELENGTH_RANGE = "WavelengthRange"


class SumType:
    IN_LAMBDA = "SumInLambda"
    IN_Q = "SumInQ"


class SubalgLogging:
    OFF = "Logging OFF"
    ON = "Logging ON"


class ReflectometryILLSumForeground(DataProcessorAlgorithm):
    def category(self):
        """Returns algorithm's categories."""
        return "ILL\\Reflectometry;Workflow\\Reflectometry"

    def name(self):
        """Returns the name of the algorithm."""
        return "ReflectometryILLSumForeground"

    def summary(self):
        """Returns a summary of the algorithm."""
        return "Sums foreground pixels in selected summation mode, optionally converting to reflectivity."

    def seeAlso(self):
        """Returns a list of related algorithm names."""
        return [
            "ReflectometryILLConvertToQ",
            "ReflectometryILLPolarizationCor",
            "ReflectometryILLPreprocess",
            "ReflectometryILLAutoProcess",
        ]

    def version(self):
        """Returns the version of the algorithm."""
        return 1

    def validateInputs(self):
        """Validates the algorithm's input properties."""
        issues = dict()
        ws = self.getProperty(Prop.INPUT_WS).value
        if not ws.run().hasProperty(common.SampleLogs.LINE_POSITION):
            issues[Prop.INPUT_WS] = "Must have a sample log entry called {}".format(common.SampleLogs.LINE_POSITION)
        if self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault:
            if self.getProperty(Prop.SUM_TYPE).value == SumType.IN_Q:
                issues[Prop.DIRECT_FOREGROUND_WS] = "Direct foreground workspace is needed for summing in Q."
        else:
            direct_ws = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
            if direct_ws.getNumberHistograms() != 1:
                issues[Prop.DIRECT_FOREGROUND_WS] = "The workspace should have only a single histogram. Was foreground summation forgotten?"
            if self.getProperty(Prop.DIRECT_WS).isDefault:
                issues[Prop.DIRECT_WS] = "The direct beam workspace is needed for processing the reflected workspace."
        w_range = self.getProperty(Prop.WAVELENGTH_RANGE).value
        if len(w_range) == 2 and w_range[0] >= w_range[1]:
            issues[Prop.WAVELENGTH_RANGE] = "Upper limit is smaller than the lower limit."
        if len(w_range) > 2:
            issues[Prop.WAVELENGTH_RANGE] = "The range should be in the form [min] or [min, max]."
        return issues

    def PyInit(self):
        """Initializes the input and output properties of the algorithm."""
        three_non_negative_ints = CompositeValidator()
        three_non_negative_ints.add(IntArrayLengthValidator(3))
        non_negative_ints = IntArrayBoundedValidator(lower=0)
        three_non_negative_ints.add(non_negative_ints)
        non_negative_float_array = FloatArrayBoundedValidator(lower=0.0)
        in_wavelength = WorkspaceUnitValidator("Wavelength")

        self.declareProperty(
            MatrixWorkspaceProperty(Prop.INPUT_WS, defaultValue="", direction=Direction.Input, validator=in_wavelength),
            doc="A reflected beam workspace (units wavelength).",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(Prop.OUTPUT_WS, defaultValue="", direction=Direction.Output), doc="The summed foreground workspace."
        )

        self.declareProperty(
            Prop.SUBALG_LOGGING,
            defaultValue=SubalgLogging.OFF,
            validator=StringListValidator([SubalgLogging.OFF, SubalgLogging.ON]),
            doc="Enable or disable child algorithm logging.",
        )

        self.declareProperty(
            Prop.CLEANUP,
            defaultValue=utils.Cleanup.ON,
            validator=StringListValidator([utils.Cleanup.ON, utils.Cleanup.OFF]),
            doc="Enable or disable intermediate workspace cleanup.",
        )

        self.declareProperty(
            Prop.SUM_TYPE,
            defaultValue=SumType.IN_LAMBDA,
            validator=StringListValidator([SumType.IN_LAMBDA, SumType.IN_Q]),
            doc="Type of summation to perform.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.DIRECT_FOREGROUND_WS,
                defaultValue="",
                direction=Direction.Input,
                optional=PropertyMode.Optional,
                validator=in_wavelength,
            ),
            doc="Summed direct beam workspace (units wavelength).",
        )

        self.declareProperty(
            IntArrayProperty(
                Prop.FOREGROUND_INDICES,
                values=[Property.EMPTY_INT, Property.EMPTY_INT, Property.EMPTY_INT],
                validator=three_non_negative_ints,
            ),
            doc="A three element array of foreground start, centre and end workspace indices.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.DIRECT_WS, defaultValue="", direction=Direction.Input, optional=PropertyMode.Optional, validator=in_wavelength
            ),
            doc="The (not summed) direct beam workspace (units wavelength).",
        )

        self.declareProperty(
            FloatArrayProperty(Prop.WAVELENGTH_RANGE, values=[0.0], validator=non_negative_float_array), doc="The wavelength bounds."
        )

    def PyExec(self):
        """Executes the algorithm."""
        self._subalg_logging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanup_mode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanup_mode, self._subalg_logging)
        ws_prefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(ws_prefix, cleanup_mode)

        ws = self._input_ws()

        process_reflected = not self._direct_only()
        if process_reflected:
            self._add_beam_statistics_to_logs(ws)

        sum_type = self._sum_type()
        if sum_type == SumType.IN_LAMBDA:
            ws = self._sum_foreground_in_lambda(ws)
        else:
            ws = self._divide_by_direct(ws)
            ws = self._sum_foreground_in_q(ws)
        ws.run().addProperty(common.SampleLogs.SUM_TYPE, sum_type, True)
        ws = self._apply_wavelength_range(ws)

        if process_reflected and sum_type == SumType.IN_LAMBDA:
            self._rebin_direct_to_reflected(self.getProperty(Prop.DIRECT_FOREGROUND_WS).value, ws)

        self._finalize(ws)

    def _add_beam_statistics_to_logs(self, ws: MatrixWorkspace) -> None:
        """Calculates beam statistics and add the results to the sample logs.

        Keyword arguments:
        ws -- target workspace for adding beam statistics to sample logs
        """
        reflected_foreground = self._foreground_indices(ws)
        direct_ws = self.getProperty(Prop.DIRECT_WS).value
        direct_foreground = self._foreground_indices(direct_ws)
        instrument_name = common.instrument_name(ws)
        pixel_size = common.pixel_size(instrument_name)
        det_resolution = common.detector_resolution()
        ReflectometryBeamStatistics(
            ReflectedBeamWorkspace=ws,
            ReflectedForeground=reflected_foreground,
            DirectLineWorkspace=direct_ws,
            DirectForeground=direct_foreground,
            PixelSize=pixel_size,
            DetectorResolution=det_resolution,
            FirstSlitName="slit2",
            FirstSlitSizeSampleLog=common.SampleLogs.SLIT2WIDTH,
            SecondSlitName="slit3",
            SecondSlitSizeSampleLog=common.SampleLogs.SLIT3WIDTH,
            EnableLogging=self._subalg_logging,
        )

    def _apply_wavelength_range(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Cuts wavelengths outside the wavelength range from a TOF workspace.

        Keyword arguments:
        ws -- workspace to be cropped to a given wavelength range
        """
        w_range = self.getProperty(Prop.WAVELENGTH_RANGE).value
        range_prop = {"XMin": w_range[0]}
        if len(w_range) == 2:
            range_prop["XMax"] = w_range[1]
        cropped_ws_name = self._names.withSuffix("cropped")
        cropped_ws = CropWorkspace(InputWorkspace=ws, OutputWorkspace=cropped_ws_name, EnableLogging=self._subalg_logging, **range_prop)
        self._cleanup.cleanup(ws)
        return cropped_ws

    def _correct_for_fractional_foreground_centre(self, ws: MatrixWorkspace, summed_foreground: MatrixWorkspace) -> MatrixWorkspace:
        """Corrects the position of the foreground centre of the provided workspace by the fractional position index
        coming from the fit, and then rotates the instrument so that it always faces the sample.

        This method needs to be called after having summed the foreground but before transferring to momentum transfer.
        This needs to be called in both coherent and incoherent cases, regardless of the angle calibration option.
        The reason for this is that up to this point is the fractional workspace index that corresponds to the calibrated 2theta.
        However, the momentum transfer calculation, which normally comes after summing the foreground, takes the 2theta
        from the spectrumInfo of the summed foreground workspace. Hence the code below translated the detector by
        the difference of the fractional and integer foreground centre along the detector plane, it also applies local
        rotation so that the detector continues to face the sample.
        Note that this translation has nothing to do with the difference of foreground centres in direct and reflected beams,
        which is handled already in the pre-process algorithm. Here, the processing only takes care of the difference
        between the fractional and integer foreground centre of the reflected beam with already calibrated angle.
        Note also, that this could probably be avoided, if the loader placed the integer foreground at the given angle,
        and not the fractional one. Fractional foreground centre only matter when calculating the difference between
        the direct and reflected beams. For the final Q (and sigma) calculation, it takes the position/angle from
        the spectrumInfo()...(0), which corresponds to the centre of the pixel.

        Keyword arguments:
        ws -- workspace used as a source of foreground information
        summed_foreground -- workspace containing summed foreground to be corrected
        """
        foreground = self._foreground_indices(ws)
        # integer foreground centre
        beam_pos_index = foreground[1]
        # fractional foreground centre
        line_position = ws.run().getProperty(common.SampleLogs.LINE_POSITION).value
        l2 = ws.run().getProperty("L2").value
        instr = common.instrument_name(ws)
        pixel_size = common.pixel_size(instr)
        # the distance between the fractional and integer foreground centres along the detector plane
        dist = pixel_size * (line_position - beam_pos_index)
        if dist != 0.0:
            det_point_1 = ws.spectrumInfo().position(0)
            det_point_2 = ws.spectrumInfo().position(20)
            beta = numpy.atan2((det_point_2[0] - det_point_1[0]), (det_point_2[2] - det_point_1[2]))
            x_vs_y = numpy.sin(beta) * dist
            mz = numpy.cos(beta) * dist
            if instr == "D17":
                mx = x_vs_y
                my = 0.0
                rotation_axis = [0, 1, 0]
            else:
                mx = 0.0
                my = x_vs_y
                rotation_axis = [-1, 0, 0]
            MoveInstrumentComponent(Workspace=summed_foreground, ComponentName="detector", X=mx, Y=my, Z=mz, RelativePosition=True)
            angle_corr = numpy.arctan2(dist, l2) * 180 / numpy.pi
            RotateInstrumentComponent(
                Workspace=summed_foreground,
                ComponentName="detector",
                X=rotation_axis[0],
                Y=rotation_axis[1],
                Z=rotation_axis[2],
                Angle=angle_corr,
                RelativeRotation=True,
            )
        return summed_foreground

    def _direct_only(self) -> bool:
        """Returns true only if the direct beam should be processed."""
        return self.getProperty(Prop.DIRECT_FOREGROUND_WS).isDefault

    def _divide_by_direct(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Divides the provided workspace by the direct beam workspace.

        Keyword arguments:
        ws -- reflected beam workspace to be divided
        """
        direct_ws = self._rebin_direct_to_reflected(self.getProperty(Prop.DIRECT_FOREGROUND_WS).value, ws)
        reflectivity_ws_name = self._names.withSuffix("reflectivity")
        reflectivity_ws = Divide(
            LHSWorkspace=ws, RHSWorkspace=direct_ws, OutputWorkspace=reflectivity_ws_name, EnableLogging=self._subalg_logging
        )
        self._cleanup.cleanup(ws)
        reflectivity_ws.setYUnit("Reflectivity")
        reflectivity_ws.setYUnitLabel("Reflectivity")
        return reflectivity_ws

    def _finalize(self, ws: MatrixWorkspace) -> None:
        """Sets workspace ws to OutputWorkspace and initiate clean up.

        Keyword arguments:
        ws -- workspace to be set as output
        """
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _foreground_indices(self, ws: MatrixWorkspace) -> List[int]:
        """Returns a three-element list of foreground start, center and end workspace indices.

        Keyword arguments:
        ws -- workspace used as source of foreground indices metadata
        """
        foregroundProp = self.getProperty(Prop.FOREGROUND_INDICES)
        if not foregroundProp.isDefault:
            return foregroundProp.value
        logs = ws.run()
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_START):
            raise RuntimeError("The sample logs are missing the '{} entry".format(common.SampleLogs.FOREGROUND_START))
        start = logs.getProperty(common.SampleLogs.FOREGROUND_START).value
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_CENTRE):
            raise RuntimeError("The sample logs are missing the '{}' entry.".format(common.SampleLogs.FOREGROUND_CENTRE))
        centre = logs.getProperty(common.SampleLogs.FOREGROUND_CENTRE).value
        if not logs.hasProperty(common.SampleLogs.FOREGROUND_END):
            raise RuntimeError("The sample logs are missing the '{}' entry.".format(common.SampleLogs.FOREGROUND_END))
        end = logs.getProperty(common.SampleLogs.FOREGROUND_END).value
        return [start, centre, end]

    def _input_ws(self) -> MatrixWorkspace:
        """Returns the input workspaces."""
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        return ws

    def _rebin_direct_to_reflected(self, direct_ws: MatrixWorkspace, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Rebins direct_ws foreground to ws, returns the result of rebinning.

        Keyword arguments:
        direct_ws -- direct workspace to be rebinned
        ws -- reflected workspace used as WorkspaceToMatch for direct workspace
        """
        rebinned_direct_ws_name = "{}_rebinned".format(direct_ws.name())
        rebinned_ws = RebinToWorkspace(
            WorkspaceToRebin=direct_ws, WorkspaceToMatch=ws, OutputWorkspace=rebinned_direct_ws_name, EnableLogging=self._subalg_logging
        )
        return rebinned_ws

    def _sum_foreground_in_lambda(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Sum the foreground region into a single histogram.

        Keyword arguments:
        ws -- workspace used as a source of the foreground spectrum
        """
        foreground = self._foreground_indices(ws)
        sum_indices = [i for i in range(foreground[0], foreground[2] + 1)]
        beam_pos_index = foreground[1]
        foreground_ws_name = self._names.withSuffix("grouped")
        foreground_ws = ExtractSingleSpectrum(
            InputWorkspace=ws, OutputWorkspace=foreground_ws_name, WorkspaceIndex=beam_pos_index, EnableLogging=self._subalg_logging
        )
        max_index = ws.getNumberHistograms() - 1
        for i in sum_indices:
            if i == beam_pos_index:
                continue
            if i < 0 or i > max_index:
                self.log().warning("Foreground partially out of the workspace.")
            addee_ws_name = self._names.withSuffix("addee")
            addee_ws = ExtractSingleSpectrum(
                InputWorkspace=ws, OutputWorkspace=addee_ws_name, WorkspaceIndex=i, EnableLogging=self._subalg_logging
            )
            addee_ws = RebinToWorkspace(
                WorkspaceToRebin=addee_ws, WorkspaceToMatch=foreground_ws, OutputWorkspace=addee_ws_name, EnableLogging=self._subalg_logging
            )
            Plus(LHSWorkspace=foreground_ws_name, RHSWorkspace=addee_ws_name, OutputWorkspace=foreground_ws_name)
            self._cleanup.cleanup(addee_ws)
        foreground_ws = self._correct_for_fractional_foreground_centre(ws, foreground_ws)
        self._cleanup.cleanup(ws)
        return foreground_ws

    def _sum_foreground_in_q(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Sum the foreground region into a single histogram using the coherent method.

        Keyword arguments:
        ws -- workspace to be summed in momentum exchange
        """
        foreground = self._foreground_indices(ws)
        sum_indices = [i for i in range(foreground[0], foreground[2] + 1)]
        line_position = ws.run().getProperty(common.SampleLogs.LINE_POSITION).value
        is_flat_sample = not ws.run().getProperty("beam_stats.bent_sample").value
        sum_ws_name = self._names.withSuffix("summed_in_Q")
        sum_ws = ReflectometrySumInQ(
            InputWorkspace=ws,
            OutputWorkspace=sum_ws_name,
            InputWorkspaceIndexSet=sum_indices,
            BeamCentre=line_position,
            FlatSample=is_flat_sample,
            EnableLogging=self._subalg_logging,
        )
        sum_ws = self._correct_for_fractional_foreground_centre(ws, sum_ws)
        self._cleanup.cleanup(ws)
        return sum_ws

    def _sum_type(self) -> str:
        return self.getProperty(Prop.SUM_TYPE).value


AlgorithmFactory.subscribe(ReflectometryILLSumForeground)
