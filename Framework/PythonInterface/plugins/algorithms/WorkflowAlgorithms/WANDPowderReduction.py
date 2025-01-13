# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    AlgorithmFactory,
    AnalysisDataService,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
    IEventWorkspace,
    WorkspaceProperty,
    WorkspaceGroup,
    ADSValidator,
)
from mantid.dataobjects import MaskWorkspaceProperty
from mantid.simpleapi import (
    ConvertSpectrumAxis,
    ConjoinWorkspaces,
    Transpose,
    ResampleX,
    CopyInstrumentParameters,
    Divide,
    DeleteWorkspace,
    Scale,
    MaskAngle,
    ExtractMask,
    Minus,
    SumSpectra,
    ExtractUnmaskedSpectra,
    mtd,
    BinaryOperateMasks,
    Integration,
    GroupWorkspaces,
    RenameWorkspace,
    GroupDetectors,
)
from mantid.kernel import (
    StringListValidator,
    Direction,
    Elastic,
    Property,
    FloatBoundedValidator,
    IntBoundedValidator,
    StringArrayProperty,
    UnitConversion,
)
import numpy as np


class WANDPowderReduction(DataProcessorAlgorithm):
    temp_workspace_list = [
        "__ws_conjoined",
        "_ws_cal",
        "_ws_cal_resampled",
        "_ws_tmp",
        "_ws_resampled",
    ]

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["LoadWAND", "SaveFocusedXYE"]

    def summary(self):
        return "Performs powder diffraction data reduction for WAND"

    def name(self):
        return "WANDPowderReduction"

    def validateInputs(self):
        issues = dict()
        return issues

    def PyInit(self):
        self.declareProperty(
            StringArrayProperty(
                "InputWorkspace",
                "",
                direction=Direction.Input,
                validator=ADSValidator(),
            ),
            doc="The main input workspace[s].",
        )

        self.declareProperty(
            "FilteredInput",
            False,
            doc="Specifies whether the input workspace was obtained from event filtering.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                "BackgroundWorkspace",
                "",
                optional=PropertyMode.Optional,
                direction=Direction.Input,
            ),
            doc="The background workspace to be subtracted.",
        )

        self.declareProperty(
            MatrixWorkspaceProperty(
                "CalibrationWorkspace",
                "",
                optional=PropertyMode.Optional,
                direction=Direction.Input,
            ),
            doc="The calibration (vanadium) workspace.",
        )

        self.declareProperty(
            "BackgroundScale",
            1.0,
            validator=FloatBoundedValidator(0.0),
            doc="The background will be scaled by this number before being subtracted.",
        )

        self.declareProperty(
            MaskWorkspaceProperty(
                "MaskWorkspace",
                "",
                optional=PropertyMode.Optional,
                direction=Direction.Input,
            ),
            doc="The mask from this workspace will be applied before reduction",
        )

        self.declareProperty(
            "Wavelength",
            1.4865,  # A
            FloatBoundedValidator(lower=0.0),  # must be positive
            "Wavelength to set the workspace (A)",
        )

        self.declareProperty(
            "Target",
            "",
            StringListValidator(["Theta", "ElasticQ", "ElasticDSpacing"]),
            "The unit to which spectrum axis is converted to",
        )

        self.copyProperties("ResampleX", ["XMin", "XMax", "LogBinning"])

        self.declareProperty(
            "NumberBins",
            1000,
            IntBoundedValidator(lower=1),  # need at least one bin
            "Number of bins to split up each spectrum into.",
        )

        self.declareProperty(
            "NormaliseBy",
            "Monitor",
            StringListValidator(["None", "Time", "Monitor"]),
            "Normalise to monitor or time. ",
        )

        self.declareProperty(
            "MaskAngle",
            Property.EMPTY_DBL,
            "Phi angle above which will be masked. See :ref:`MaskAngle <algm-MaskAngle>` for details.",
        )

        self.declareProperty(
            WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Output Workspace",
        )
        self.getProperty("OutputWorkspace").setDisableReplaceWSButton(True)

        self.declareProperty(
            "Sum",
            False,
            doc="Specifies either single output workspace or output group workspace containing several workspaces.",
        )

    def PyExec(self):
        data = self._expand_groups()

        bkg = self.getProperty("BackgroundWorkspace").valueAsStr  # same background for all
        cal = self.getProperty("CalibrationWorkspace").value  # same calibration for all
        numberBins = self.getProperty("NumberBins").value
        outWS = self.getPropertyValue("OutputWorkspace")
        summing = self.getProperty("Sum").value  # [Yes or No]
        filtered_eve = self.getProperty("FilteredInput").value  # [Yes or No]

        # convert all of the input workspaces into spectrum of "target" units (generally angle)
        data, masks = self._convert_data(data)

        # determine x-range
        xMin, xMax = self._locate_global_xlimit(data)

        if filtered_eve:
            ExtractMask(InputWorkspace=cal, OutputWorkspace="mask_shared", EnableLogging=False)
            if cal is None:
                _ws_cal_resampled = None
            else:
                _ws_cal_resampled = self._resample_calibration(data[0], "mask_shared", xMin, xMax)

            if bkg is None or bkg.strip() == "":
                _ws_bkg_resampled = None
            else:
                _ws_bkg_resampled = self._resample_background(bkg, data[0], "mask_shared", xMin, xMax, _ws_cal_resampled)

        # BEGIN_FOR: prcess_spectra
        for n, (_wsn, _mskn) in enumerate(zip(data, masks)):
            # resample spectra
            ResampleX(
                InputWorkspace=_wsn,
                OutputWorkspace=_wsn,
                XMin=xMin,
                XMax=xMax,
                NumberBins=numberBins,
                EnableLogging=False,
            )

            if not filtered_eve:
                if cal is None:
                    _ws_cal_resampled = None
                else:
                    _ws_cal_resampled = self._resample_calibration(_wsn, _mskn, xMin, xMax)

            if _ws_cal_resampled is not None:
                Divide(
                    LHSWorkspace=_wsn,
                    RHSWorkspace=_ws_cal_resampled,
                    OutputWorkspace=_wsn,
                    EnableLogging=False,
                )

            Scale(
                InputWorkspace=_wsn,
                OutputWorkspace=_wsn,
                Factor=self._get_scale(cal) / self._get_scale(_wsn),
                EnableLogging=False,
            )

            # background
            if not filtered_eve:
                if bkg is None or bkg.strip() == "":
                    _ws_bkg_resampled = None
                else:
                    _ws_bkg_resampled = self._resample_background(bkg, _wsn, _mskn, xMin, xMax, _ws_cal_resampled)

            if _ws_bkg_resampled is not None:
                Minus(
                    LHSWorkspace=_wsn,
                    RHSWorkspace=_ws_bkg_resampled,
                    OutputWorkspace=_wsn,
                    EnableLogging=False,
                )

            if summing:
                # conjoin
                if n < 1:
                    RenameWorkspace(
                        InputWorkspace=_wsn,
                        OutputWorkspace="__ws_conjoined",
                        EnableLogging=False,
                    )
                else:
                    # this adds to `InputWorkspace1`
                    ConjoinWorkspaces(
                        InputWorkspace1="__ws_conjoined",
                        InputWorkspace2=_wsn,
                        CheckOverlapping=False,
                        EnableLogging=False,
                    )

        # END_FOR: prcess_spectra
        # Step_3: sum all spectra
        # ref: https://docs.mantidproject.org/nightly/algorithms/SumSpectra-v1.html
        if summing:
            if cal is not None:
                outWS = SumSpectra(
                    InputWorkspace="__ws_conjoined",
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=not bool(cal),
                    EnableLogging=False,
                )
            else:
                outWS = SumSpectra(
                    InputWorkspace="__ws_conjoined",
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=True,
                    EnableLogging=False,
                )
        else:
            if len(data) == 1:
                outWS = RenameWorkspace(InputWorkspace=data[0], OutputWorkspace=outWS)
            else:
                outWS = GroupWorkspaces(InputWorkspaces=data, OutputWorkspace=outWS)

        self.setProperty("OutputWorkspace", outWS)

        # Step_4: remove temp workspaces
        [DeleteWorkspace(ws, EnableLogging=False) for ws in self.temp_workspace_list if mtd.doesExist(ws)]

    def _expand_groups(self):
        """expand workspace groups"""
        workspaces = self.getProperty("InputWorkspace").value
        input_workspaces = []
        for wsname in workspaces:
            wks = AnalysisDataService.retrieve(wsname)
            if isinstance(wks, WorkspaceGroup):
                input_workspaces.extend(wks.getNames())
            else:
                input_workspaces.append(wsname)

        return input_workspaces

    def _get_scale(self, x):
        """return the scale factor needed during normalization"""
        normaliseBy = self.getProperty("NormaliseBy").value

        if x is None:
            return 1
        else:
            if str(normaliseBy).lower() == "none":
                return 1
            elif str(normaliseBy).lower() == "monitor":
                return mtd[str(x)].run().getProtonCharge()
            elif str(normaliseBy).lower() == "time":
                return mtd[str(x)].run().getLogData("duration").value
            else:
                raise ValueError(f"Unknown normalize type: {normaliseBy}")

    def _to_spectrum_axis(self, workspace_in, workspace_out, mask, instrument_donor=None):
        target = self.getProperty("Target").value
        wavelength = self.getProperty("Wavelength").value
        e_fixed = UnitConversion.run("Wavelength", "Energy", wavelength, 0, 0, 0, Elastic, 0)
        filtered_eve = self.getProperty("FilteredInput").value

        if instrument_donor or not filtered_eve:
            ExtractUnmaskedSpectra(
                InputWorkspace=workspace_in,
                OutputWorkspace=workspace_out,
                MaskWorkspace=mask,
                EnableLogging=False,
            )

        if instrument_donor:
            wksp_tmp = workspace_out
        else:
            wksp_tmp = workspace_in

        if isinstance(mtd[wksp_tmp], IEventWorkspace):
            Integration(
                InputWorkspace=wksp_tmp,
                OutputWorkspace=workspace_out,
                EnableLogging=False,
            )

        if instrument_donor:
            CopyInstrumentParameters(
                InputWorkspace=instrument_donor,
                OutputWorkspace=workspace_out,
                EnableLogging=False,
            )

        if not filtered_eve:
            ConvertSpectrumAxis(
                InputWorkspace=workspace_out,
                OutputWorkspace=workspace_out,
                Target=target,
                EFixed=e_fixed,
                EnableLogging=False,
            )

        # this checks for any duplicated values in target axis, if
        # so then group them together
        axis_values = mtd[workspace_out].getAxis(1).extractValues()
        equal_values = axis_values == np.roll(axis_values, -1)
        if np.any(equal_values):
            operator = np.full_like(equal_values, ",", dtype="<U1")
            operator[equal_values] = "+"
            grouping_pattern = "".join(str(n) + op for n, op in enumerate(operator))
            GroupDetectors(
                InputWorkspace=workspace_out, OutputWorkspace=workspace_out, GroupingPattern=grouping_pattern, EnableLogging=False
            )
            ConvertSpectrumAxis(
                InputWorkspace=workspace_out,
                OutputWorkspace=workspace_out,
                Target=target,
                EFixed=e_fixed,
                EnableLogging=False,
            )

        Transpose(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            EnableLogging=False,
        )

        return workspace_out

    def _to_spectrum_axis_resample(self, workspace_in, workspace_out, mask, instrument_donor, x_min, x_max):
        # common part of converting axis
        self._to_spectrum_axis(workspace_in, workspace_out, mask, instrument_donor)

        # rebin the data
        number_bins = self.getProperty("NumberBins").value
        return ResampleX(
            InputWorkspace=workspace_out,
            OutputWorkspace=workspace_out,
            XMin=x_min,
            XMax=x_max,
            NumberBins=number_bins,
            EnableLogging=False,
        )

    def _convert_data(self, input_workspaces):
        mask = self.getProperty("MaskWorkspace").value
        mask_angle = self.getProperty("MaskAngle").value
        outname = self.getProperty("OutputWorkspace").valueAsStr

        # NOTE:
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.

        # BEGIN_FOR: located_global_xMin&xMax
        output_workspaces = list()
        for n, in_wksp in enumerate(input_workspaces):
            try:
                temp_val = mtd[in_wksp].run().getTimeAveragedValue("HB2C:SE:SampleTemp")
            except RuntimeError:
                temp_val = 300.0

            if temp_val == 0.0:
                temp_val = 300.0
            temp_val = "{:.1F}".format(temp_val).replace(".", "p")
            out_tmp = f"{outname}{n + 1}_T{temp_val}K"
            output_workspaces.append(out_tmp)
        mask_workspaces = []
        for n, (_wksp_in, _wksp_out) in enumerate(zip(input_workspaces, output_workspaces)):
            _wksp_in = str(_wksp_in)
            if mask_angle == Property.EMPTY_DBL:
                self._to_spectrum_axis(_wksp_in, _wksp_out, mask)
                mask_workspaces.append(mask)
            else:
                _mask_n = f"__mask_{n}"  # mask for n-th
                self.temp_workspace_list.append(_mask_n)  # cleanup later

                ExtractMask(InputWorkspace=_wksp_in, OutputWorkspace=_mask_n, EnableLogging=False)
                if mask_angle != Property.EMPTY_DBL:
                    MaskAngle(
                        Workspace=_mask_n,
                        MinAngle=mask_angle,
                        Angle="Phi",
                        EnableLogging=False,
                    )
                if mask is not None:
                    # might be a bug if the mask angle isn't set
                    BinaryOperateMasks(
                        InputWorkspace1=_mask_n,
                        InputWorkspace2=mask,
                        OperationType="OR",
                        OutputWorkspace=_mask_n,
                        EnableLogging=False,
                    )

                self._to_spectrum_axis(_wksp_in, _wksp_out, _mask_n)

                # append to the list of processed workspaces
                mask_workspaces.append(_mask_n)

        return output_workspaces, mask_workspaces

    def _locate_global_xlimit(self, workspaces):
        """Find the global bin from all spectrum"""
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.

        # use the supplied start value
        if self.getProperty("xMin").isDefault:
            _xMin = 1e16
        else:
            _xMin = self.getProperty("XMin").value
        if self.getProperty("xMax").isDefault:
            _xMax = -1e16
        else:
            _xMax = self.getProperty("XMax").value
        # if both were set there is nothing to do
        if _xMin < _xMax and _xMin < 1e16 and _xMax > -1e16:
            return _xMin, _xMax

        # update values based on all workspaces
        for name in workspaces:
            _ws_tmp = mtd[name]
            _xMin = min(_xMin, _ws_tmp.readX(0).min())
            _xMax = max(_xMax, _ws_tmp.readX(0).max())

        return _xMin, _xMax

    def _resample_calibration(
        self,
        current_workspace,
        mask_name,
        x_min,
        x_max,
    ):
        """Perform resample on calibration"""
        cal = self.getProperty("CalibrationWorkspace").valueAsStr

        return self._to_spectrum_axis_resample(cal, "_ws_cal", mask_name, current_workspace, x_min, x_max)

    def _resample_background(
        self,
        current_background,
        current_workspace,
        make_name,
        x_min,
        x_max,
        resampled_calibration,
    ):
        """Perform resample on given background"""

        # create unique name for this background
        outname = str(current_background) + str(current_workspace)
        self.temp_workspace_list.append(outname)

        self._to_spectrum_axis_resample(current_background, outname, make_name, current_workspace, x_min, x_max)

        if resampled_calibration:
            Divide(
                LHSWorkspace=outname,
                RHSWorkspace=resampled_calibration,
                OutputWorkspace=outname,
                EnableLogging=False,
            )

        cal = self.getProperty("CalibrationWorkspace").valueAsStr
        Scale(
            InputWorkspace=outname,
            OutputWorkspace=outname,
            Factor=self._get_scale(cal) / self._get_scale(current_background),
            EnableLogging=False,
        )

        Scale(
            InputWorkspace=outname,
            OutputWorkspace=outname,
            Factor=self.getProperty("BackgroundScale").value,
            EnableLogging=False,
        )

        return outname


AlgorithmFactory.subscribe(WANDPowderReduction)
