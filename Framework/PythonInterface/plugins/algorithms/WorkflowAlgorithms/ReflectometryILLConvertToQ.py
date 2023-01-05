# -*- coding: utf-8 -*-# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import ILL_utilities as utils
from mantid.api import (
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    MatrixWorkspace,
    PropertyMode,
    Run,
    WorkspaceUnitValidator,
)
from mantid.kernel import Direction, FloatBoundedValidator, Property, StringListValidator
from mantid.simpleapi import (
    CloneWorkspace,
    ConvertAxisByFormula,
    ConvertToPointData,
    CreateWorkspace,
    Divide,
    GroupToXResolution,
    Multiply,
    ReflectometryMomentumTransfer,
)
import ReflectometryILL_common as common
import scipy.constants as constants
import numpy as np
from typing import List, Tuple


class Prop:
    CLEANUP = "Cleanup"
    DIRECT_FOREGROUND_WS = "DirectForegroundWorkspace"
    GROUPING_FRACTION = "GroupingQFraction"
    INPUT_WS = "InputWorkspace"
    OUTPUT_WS = "OutputWorkspace"
    SUBALG_LOGGING = "SubalgorithmLogging"
    THETA_CORRECTION = "ThetaCorrection"


class SubalgLogging:
    OFF = "Logging OFF"
    ON = "Logging ON"


class ReflectometryILLConvertToQ(DataProcessorAlgorithm):
    @staticmethod
    def _foreground(sample_logs: Run) -> List[float]:
        """Returns a [start, end] list defining the foreground workspace indices from provided sample logs metadata.

        Keyword arguments:
        sample_logs -- metadata used as source of foreground information
        """
        start = sample_logs.getProperty(common.SampleLogs.FOREGROUND_START).value
        end = sample_logs.getProperty(common.SampleLogs.FOREGROUND_END).value
        return [start, end]

    @staticmethod
    def _sum_type(sample_logs: Run) -> float:
        """Returns the sum type applied to ws from metadata.

        Keyword arguments:
        logs -- source of metadata
        """
        return sample_logs.getProperty(common.SampleLogs.SUM_TYPE).value

    @staticmethod
    def _tof_channel_width(sample_logs: Run) -> float:
        """Returns the time of flight bin width from metadata.

        Keyword arguments:
        sample_logs -- source of metadata
        """
        return sample_logs.getProperty("PSD.time_of_flight_0").value

    def category(self):
        """Returns algorithm's categories."""
        return "ILL\\Reflectometry;Workflow\\Reflectometry"

    def name(self):
        """Returns the name of the algorithm."""
        return "ReflectometryILLConvertToQ"

    def summary(self):
        """Returns a summary of the algorithm."""
        return "Converts a reflectivity workspace from wavelength to momentum transfer."

    def seeAlso(self):
        """Returns a list of related algorithm names."""
        return [
            "ReflectometryILLPolarizationCor",
            "ReflectometryILLPreprocess",
            "ReflectometryILLSumForeground",
            "ReflectometryMomentumTransfer",
            "ReflectometryILLAutoProcess",
        ]

    def version(self):
        """Returns the version of the algorithm."""
        return 1

    def PyExec(self):
        """Executes the algorithm."""
        self._subalg_logging = self.getProperty(Prop.SUBALG_LOGGING).value == SubalgLogging.ON
        cleanup_mode = self.getProperty(Prop.CLEANUP).value
        self._cleanup = utils.Cleanup(cleanup_mode, self._subalg_logging)
        ws_prefix = self.getPropertyValue(Prop.OUTPUT_WS)
        self._names = utils.NameSource(ws_prefix, cleanup_mode)

        ws, direct_ws = self._input_ws()

        ws = self._correct_for_chopper_openings(ws, direct_ws)
        ws = self._convert_to_momentum_transfer(ws)
        if not self.getProperty("ThetaCorrection").isDefault:
            theta_ws = self.getProperty("ThetaCorrection").value
            theta_ws_in_q = CloneWorkspace(InputWorkspace=theta_ws, OutputWorkspace="{}_in_Q".format(theta_ws.name()))
            theta0 = ws.spectrumInfo().twoTheta(0) / 2.0
            theta_ws_in_q = ConvertAxisByFormula(
                InputWorkspace=theta_ws_in_q,
                OutputWorkspace=theta_ws_in_q.name(),
                Axis="X",
                Formula="4*pi*{}/x".format(np.sin(theta0)),
                AxisUnits="MomentumTransfer",
            )
            theta_ws_in_q.setDx(0, ws.readDx(0))
            theta_ws_in_q = self._to_point_data(theta_ws_in_q)
            theta_ws_in_q = self._group_points(theta_ws_in_q, "theta_")
            self._cleanup.cleanupLater(theta_ws_in_q)
        sum_in_lambda = self._sum_type(ws.run()) == common.SUM_IN_LAMBDA
        if sum_in_lambda:
            direct_ws = self._same_q_and_dq(ws, direct_ws, "direct_")

        ws = self._to_point_data(ws)
        ws = self._group_points(ws)

        if sum_in_lambda:
            direct_ws = self._to_point_data(direct_ws, "direct_")
            direct_ws = self._group_points(direct_ws, "direct_")
            ws = self._divide_by_direct(ws, direct_ws)

        if not self.getProperty("ThetaCorrection").isDefault:
            ws.setX(0, ws.readX(0) * theta_ws_in_q.readY(0))

        self._finalize(ws)

    def PyInit(self):
        """Initializes the input and output properties of the algorithm."""
        positive_float = FloatBoundedValidator(lower=0.0, exclusive=True)
        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.INPUT_WS, defaultValue="", direction=Direction.Input, validator=WorkspaceUnitValidator("Wavelength")
            ),
            doc="A reflectivity workspace in wavelength to be converted to Q.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(Prop.OUTPUT_WS, defaultValue="", direction=Direction.Output),
            doc="The input workspace in momentum transfer.",
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
            MatrixWorkspaceProperty(
                Prop.DIRECT_FOREGROUND_WS, defaultValue="", direction=Direction.Input, validator=WorkspaceUnitValidator("Wavelength")
            ),
            doc="Summed direct beam workspace.",
        )

        self.declareProperty(
            Prop.GROUPING_FRACTION,
            defaultValue=Property.EMPTY_DBL,
            validator=positive_float,
            doc="If set, group the output by steps of this fraction multiplied by Q resolution",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                Prop.THETA_CORRECTION,
                defaultValue="",
                direction=Direction.Input,
                optional=PropertyMode.Optional,
                validator=WorkspaceUnitValidator("Wavelength"),
            ),
            doc="Theta correction factors from gravity correction.",
        )

    def validateInputs(self):
        """Validates the input properties."""
        issues = dict()
        input_ws = self.getProperty(Prop.INPUT_WS).value
        if input_ws.getNumberHistograms() != 1:
            issues[Prop.INPUT_WS] = "The workspace should have only a single histogram. Was foreground summation forgotten?"
        direct_ws = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        if direct_ws.getNumberHistograms() != 1:
            issues[Prop.DIRECT_FOREGROUND_WS] = "The workspace should have only a single histogram. Was foreground summation forgotten?"
        run = input_ws.run()
        if not run.hasProperty(common.SampleLogs.SUM_TYPE):
            issues[Prop.INPUT_WS] = "'{}' entry missing in sample logs".format(common.SampleLogs.SUM_TYPE)
        else:
            sum_type = run.getProperty(common.SampleLogs.SUM_TYPE).value
            if sum_type not in [common.SUM_IN_LAMBDA, common.SUM_IN_Q]:
                issues[Prop.INPUT_WS] = "Unknown sum type in sample logs: '{}'. Allowed values: 'SumInLambda' or 'SumInQ'.".format(sum_type)
            else:
                if sum_type == common.SUM_IN_LAMBDA:
                    if direct_ws.blocksize() != input_ws.blocksize():
                        issues[Prop.DIRECT_FOREGROUND_WS] = "Number of bins does not match with InputWorkspace."
                    direct_xs = direct_ws.readX(0)
                    input_xs = input_ws.readX(0)
                    if direct_xs[0] != input_xs[0] or direct_xs[-1] != input_xs[-1]:
                        issues[Prop.DIRECT_FOREGROUND_WS] = "Binning does not match with InputWorkspace."
        return issues

    def _convert_to_momentum_transfer(self, ws: MatrixWorkspace) -> MatrixWorkspace:
        """Converts the X units of ws to momentum transfer using ReflectometryMomentumTransfer algorithm, and returns
        the result of the conversion.

        Keyword arguments:
        ws -- workspace to have its X axis converted to momentum transfer
        """
        logs = ws.run()
        reflected_foreground = self._foreground(logs)
        instrument = ws.getInstrument()
        instrument_name = common.instrument_name(ws)
        sum_type = logs.getProperty(common.SampleLogs.SUM_TYPE).value
        pixel_size = common.pixel_size(instrument_name)
        det_resolution = common.detector_resolution()
        chopper_speed = common.chopper_speed(logs, instrument)
        chopper_opening = common.chopper_opening_angle(logs, instrument)
        chopper_radius = instrument.getNumberParameter("chopper_radius")[0]
        chopper_pair_dist = common.chopper_pair_distance(logs, instrument)
        tof_bin_width = self._tof_channel_width(logs)
        q_ws_name = self._names.withSuffix("in_momentum_transfer")
        q_ws = ReflectometryMomentumTransfer(
            InputWorkspace=ws,
            OutputWorkspace=q_ws_name,
            SummationType=sum_type,
            ReflectedForeground=reflected_foreground,
            PixelSize=pixel_size,
            DetectorResolution=det_resolution,
            ChopperSpeed=chopper_speed,
            ChopperOpening=chopper_opening,
            ChopperRadius=chopper_radius,
            ChopperPairDistance=chopper_pair_dist,
            FirstSlitName="slit2",
            FirstSlitSizeSampleLog=common.SampleLogs.SLIT2WIDTH,
            SecondSlitName="slit3",
            SecondSlitSizeSampleLog=common.SampleLogs.SLIT3WIDTH,
            TOFChannelWidth=tof_bin_width,
            EnableLogging=self._subalg_logging,
        )
        self._cleanup.cleanup(ws)
        return q_ws

    def _correct_for_chopper_openings(self, ws: MatrixWorkspace, direct_ws: MatrixWorkspace) -> MatrixWorkspace:
        """Corrects reflectivity values if chopper openings between RB and DB differ.

        Keyword arguments:
        ws -- workspace to be corrected
        direct_ws -- direct beam workspace
        """

        def opening(instr, logs, x_s):
            chopperGap = common.chopper_pair_distance(logs, instr)
            chopperPeriod = 60.0 / common.chopper_speed(logs, instr)
            openingAngle = common.chopper_opening_angle(logs, instr)
            return chopperGap * constants.m_n / constants.h / chopperPeriod * x_s * 1e-10 + openingAngle / 360.0

        instrument = ws.getInstrument()
        x_bins = ws.readX(0)
        xs = (x_bins[:-1] + x_bins[1:]) / 2.0
        reflected_opening = opening(instrument, ws.run(), xs)
        direct_opening = opening(instrument, direct_ws.run(), xs)
        cor_factor_ws_name = self._names.withSuffix("chopper_opening_correction_factors")
        cor_factor_ws = CreateWorkspace(
            OutputWorkspace=cor_factor_ws_name,
            DataX=x_bins,
            DataY=direct_opening / reflected_opening,
            UnitX=ws.getAxis(0).getUnit().unitID(),
            ParentWorkspace=ws,
            EnableLogging=self._subalg_logging,
        )
        corrected_ws_name = self._names.withSuffix("corrected_by_chopper_opening")
        corrected_ws = Multiply(
            LHSWorkspace=ws, RHSWorkspace=cor_factor_ws, OutputWorkspace=corrected_ws_name, EnableLogging=self._subalg_logging
        )
        self._cleanup.cleanup(cor_factor_ws)
        self._cleanup.cleanup(ws)
        return corrected_ws

    def _divide_by_direct(self, ws: MatrixWorkspace, direct_ws: MatrixWorkspace) -> MatrixWorkspace:
        """Divides ws by the direct beam.

        Keyword arguments:
        ws -- workspace to be normalized to direct beam
        direct_ws -- workspace containing direct beam data
        """
        reflectivity_ws_name = self._names.withSuffix("reflectivity")
        reflectivity_ws = Divide(
            LHSWorkspace=ws, RHSWorkspace=direct_ws, OutputWorkspace=reflectivity_ws_name, EnableLogging=self._subalg_logging
        )
        self._cleanup.cleanup(direct_ws)
        reflectivity_ws.setYUnit("Reflectivity")
        reflectivity_ws.setYUnitLabel("Reflectivity")
        # The X error data is lost in Divide.
        reflectivity_ws.setDx(0, ws.readDx(0))
        self._cleanup.cleanup(ws)
        return reflectivity_ws

    def _finalize(self, ws: MatrixWorkspace) -> None:
        """Sets workspace ws to OutputWorkspace and clean up.

        Keyword arguments:
        ws -- workspace to be set as output
        """
        self.setProperty(Prop.OUTPUT_WS, ws)
        self._cleanup.cleanup(ws)
        self._cleanup.finalCleanup()

    def _group_points(self, ws: MatrixWorkspace, extra_label="") -> MatrixWorkspace:
        """Group bins by Q resolution.

        Keyword arguments:
        ws -- workspace to be grouped
        extra_label -- optional label to be added to the name of the output workspace
        """
        if self.getProperty(Prop.GROUPING_FRACTION).isDefault:
            return ws
        q_fraction = self.getProperty(Prop.GROUPING_FRACTION).value
        grouped_ws_name = self._names.withSuffix("{}grouped".format(extra_label))
        grouped_ws = GroupToXResolution(
            InputWorkspace=ws, OutputWorkspace=grouped_ws_name, FractionOfDx=q_fraction, EnableLogging=self._subalg_logging
        )
        self._cleanup.cleanup(ws)
        self._cleanup.cleanupLater(grouped_ws_name)
        return grouped_ws

    def _input_ws(self) -> Tuple[MatrixWorkspace, MatrixWorkspace]:
        """Returns the input workspace and the workspace containing direct beam data."""
        ws = self.getProperty(Prop.INPUT_WS).value
        self._cleanup.protect(ws)
        direct_ws = self.getProperty(Prop.DIRECT_FOREGROUND_WS).value
        self._cleanup.protect(direct_ws)
        return ws, direct_ws

    def _same_q_and_dq(self, ws: MatrixWorkspace, direct_ws: MatrixWorkspace, extra_label="") -> MatrixWorkspace:
        """Creates a new workspace with Y and E data coming from direct_ws and X and DX data from ws. The parent workspace
        of the output is direct_ws.

        Keyword arguments:
        ws -- source of X and DX information
        direct_ws -- source of Y and E information, parent of the output
        """
        q_ws_name = self._names.withSuffix("{}in_momentum_transfer".format(extra_label))
        q_ws = CreateWorkspace(
            OutputWorkspace=q_ws_name,
            DataX=ws.readX(0),
            DataY=direct_ws.readY(0)[::-1],  # Invert data because wavelength is inversely proportional to Q.
            DataE=direct_ws.readE(0)[::-1],
            Dx=ws.readDx(0),
            UnitX=ws.getAxis(0).getUnit().unitID(),
            ParentWorkspace=direct_ws,
            EnableLogging=self._subalg_logging,
        )
        return q_ws

    def _to_point_data(self, ws, extra_label=""):
        """Converts ws from binned to point data and returns the point data workspace.

        Keyword arguments:
        ws -- workspace to be converted
        extra_label -- optional label for the output workspace name's suffix
        """
        point_ws_name = self._names.withSuffix("{}as_points".format(extra_label))
        point_ws = ConvertToPointData(InputWorkspace=ws, OutputWorkspace=point_ws_name, EnableLogging=self._subalg_logging)
        self._cleanup.cleanup(ws)
        return point_ws


AlgorithmFactory.subscribe(ReflectometryILLConvertToQ)
