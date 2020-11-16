# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (
    ADSValidator,
    AlgorithmFactory,
    DataProcessorAlgorithm,
    MatrixWorkspaceProperty,
    PropertyMode,
    IEventWorkspace,
)
from mantid.dataobjects import MaskWorkspaceProperty
from mantid.simpleapi import (
    AnalysisDataService,
    ConvertSpectrumAxis,
    ConjoinWorkspaces,
    Transpose,
    ResampleX,
    CopyInstrumentParameters,
    CloneWorkspace,
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
)
from mantid.kernel import (
    StringListValidator,
    Direction,
    Property,
    FloatBoundedValidator,
    StringArrayProperty,
)


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
            "InputWorkspace",
            "",
            direction=Direction.Input,
            doc="The main input workspace.",
        )

        self.declareProperty(
            "BackgroundWorkspace",
            "",
            direction=Direction.Input,
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

        self.copyProperties("ConvertSpectrumAxis", ["Target", "EFixed"])

        self.copyProperties("ResampleX", ["XMin", "XMax", "NumberBins", "LogBinning"])

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
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            doc="Output Workspace",
        )
        
        self.declareProperty(
            "MultipleOutput",
            "",
            StringListValidator(["Yes", "No"]),
            doc="Specifies either single output workspace or output group workspace containing several workspaces",
        )


    def PyExec(self):
        outMult = self.getProperty("MultipleOutput").value  # [Yes or No]

        if outMult == "Yes":
            
            self.declareProperty(
                MatrixWorkspaceProperty(
                    "MatrixInputWorkspace",
                    "",
                    direction=Direction.Input
                ),
            )
            self.declareProperty(
                MatrixWorkspaceProperty(
                    "MatrixBackgroundWorkspace",
                    "",
                    optional=PropertyMode.Optional,
                    direction=Direction.Input,
                ),
            )
            
            data = self.getProperty("InputWorkspace").value
            cal = self.getProperty("CalibrationWorkspace").value
            bkg = self.getProperty("BackgroundWorkspace").value
            mask = self.getProperty("MaskWorkspace").value
            target = self.getProperty("Target").value
            eFixed = self.getProperty("EFixed").value
            xMin = self.getProperty("XMin").value
            xMax = self.getProperty("XMax").value
            numberBins = self.getProperty("NumberBins").value
            normaliseBy = self.getProperty("NormaliseBy").value
            maskAngle = self.getProperty("MaskAngle").value
            outWS = self.getPropertyValue("OutputWorkspace")

            data_scale = 1
            cal_scale = 1
            bkg_scale = 1

            self.setProperty("MatrixInputWorkspace", data)
            self.setProperty("MatrixBackgroundWorkspace", bkg)
            
            copyInput = self.getProperty("MatrixBackgroundWorkspace").value
            copyBkg = self.getProperty("MatrixBackgroundWorkspace").value

            if normaliseBy == "Monitor":
                data_scale = copyInput.run().getProtonCharge()
            elif normaliseBy == "Time":
                data_scale = copyInput.run().getLogData("duration").value

            ExtractMask(copyInput, OutputWorkspace="__mask_tmp", EnableLogging=False)

            if maskAngle != Property.EMPTY_DBL:
                MaskAngle(
                    Workspace="__mask_tmp",
                    MinAngle=maskAngle,
                    Angle="Phi",
                    EnableLogging=False,
                )

            if mask is not None:
                BinaryOperateMasks(
                    InputWorkspace1="__mask_tmp",
                    InputWorkspace2=mask,
                    OperationType="OR",
                    OutputWorkspace="__mask_tmp",
                    EnableLogging=False,
                )

            ExtractUnmaskedSpectra(
                InputWorkspace=copyInput,
                MaskWorkspace="__mask_tmp",
                OutputWorkspace="__data_tmp",
                EnableLogging=False,
            )

            if isinstance(mtd["__data_tmp"], IEventWorkspace):
                Integration(
                    InputWorkspace="__data_tmp",
                    OutputWorkspace="__data_tmp",
                    EnableLogging=False,
                )

            ConvertSpectrumAxis(
                InputWorkspace="__data_tmp",
                Target=target,
                EFixed=eFixed,
                OutputWorkspace=outWS,
                EnableLogging=False,
            )
            Transpose(
                InputWorkspace=outWS,
                OutputWorkspace=outWS, EnableLogging=False)
            ResampleX(
                InputWorkspace=outWS,
                OutputWorkspace=outWS,
                XMin=xMin,
                XMax=xMax,
                NumberBins=numberBins,
                EnableLogging=False,
            )

            if cal is not None:
                ExtractUnmaskedSpectra(
                    InputWorkspace=cal,
                    MaskWorkspace="__mask_tmp",
                    OutputWorkspace="__cal_tmp",
                    EnableLogging=False,
                )

                if isinstance(mtd["__cal_tmp"], IEventWorkspace):
                    Integration(
                        InputWorkspace="__cal_tmp",
                        OutputWorkspace="__cal_tmp",
                        EnableLogging=False,
                    )

                CopyInstrumentParameters(copyInput, "__cal_tmp", EnableLogging=False)
                ConvertSpectrumAxis(
                    InputWorkspace="__cal_tmp",
                    Target=target,
                    EFixed=eFixed,
                    OutputWorkspace="__cal_tmp",
                    EnableLogging=False,
                )
                Transpose(
                    InputWorkspace="__cal_tmp",
                    OutputWorkspace="__cal_tmp",
                    EnableLogging=False,
                )
                ResampleX(
                    InputWorkspace="__cal_tmp",
                    OutputWorkspace="__cal_tmp",
                    XMin=xMin,
                    XMax=xMax,
                    NumberBins=numberBins,
                    EnableLogging=False,
                )
                Divide(
                    LHSWorkspace=outWS,
                    RHSWorkspace="__cal_tmp",
                    OutputWorkspace=outWS,
                    EnableLogging=False,
                )

                if normaliseBy == "Monitor":
                    cal_scale = cal.run().getProtonCharge()
                elif normaliseBy == "Time":
                    cal_scale = cal.run().getLogData("duration").value

            Scale(
                InputWorkspace=outWS,
                OutputWorkspace=outWS,
                Factor=cal_scale / data_scale,
                EnableLogging=False,
            )

            if copyBkg is not None:
                ExtractUnmaskedSpectra(
                    InputWorkspace=copyBkg,
                    MaskWorkspace="__mask_tmp",
                    OutputWorkspace="__bkg_tmp",
                    EnableLogging=False,
                )

                if isinstance(mtd["__bkg_tmp"], IEventWorkspace):
                    Integration(
                        InputWorkspace="__bkg_tmp",
                        OutputWorkspace="__bkg_tmp",
                        EnableLogging=False,
                    )

                CopyInstrumentParameters(copyInput, "__bkg_tmp", EnableLogging=False)
                ConvertSpectrumAxis(
                    InputWorkspace="__bkg_tmp",
                    Target=target,
                    EFixed=eFixed,
                    OutputWorkspace="__bkg_tmp",
                    EnableLogging=False,
                )
                Transpose(
                    InputWorkspace="__bkg_tmp",
                    OutputWorkspace="__bkg_tmp",
                    EnableLogging=False,
                )
                ResampleX(
                    InputWorkspace="__bkg_tmp",
                    OutputWorkspace="__bkg_tmp",
                    XMin=xMin,
                    XMax=xMax,
                    NumberBins=numberBins,
                    EnableLogging=False,
                )

                if cal is not None:
                    Divide(
                        LHSWorkspace="__bkg_tmp",
                        RHSWorkspace="__cal_tmp",
                        OutputWorkspace="__bkg_tmp",
                        EnableLogging=False,
                    )

                if normaliseBy == "Monitor":
                    bkg_scale = copyBkg.run().getProtonCharge()

                elif normaliseBy == "Time":
                    bkg_scale = copyBkg.run().getLogData("duration").value

                Scale(
                    InputWorkspace="__bkg_tmp",
                    OutputWorkspace="__bkg_tmp",
                    Factor=cal_scale / bkg_scale,
                    EnableLogging=False,
                )
                Scale(
                    InputWorkspace="__bkg_tmp",
                    OutputWorkspace="__bkg_tmp",
                    Factor=self.getProperty("BackgroundScale").value,
                    EnableLogging=False,
                )

                Minus(
                    LHSWorkspace=outWS,
                    RHSWorkspace="__bkg_tmp",
                    OutputWorkspace=outWS,
                    EnableLogging=False,
                )

            self.setProperty("OutputWorkspace", outWS)

            # remove temp workspaces

            [
                DeleteWorkspace(ws, EnableLogging=False)
                for ws in self.temp_workspace_list
                if mtd.doesExist(ws)
            ]

        elif outMult == "No":
            
            self.declareProperty(
                StringArrayProperty(
                    "StringInputWorkspace",
                    direction=Direction.Input,
                    validator=ADSValidator(),
                ),
            )
            
            data = self.getProperty("InputWorkspace").value  # [1~n]
            bkg = self.getProperty("BackgroundWorkspace").value  # [1]
            cal = self.getProperty("CalibrationWorkspace").value  # [1]
            xMin = self.getProperty("XMin").value
            xMax = self.getProperty("XMax").value
            numberBins = self.getProperty("NumberBins").value
            outWS = self.getPropertyValue("OutputWorkspace")

            self.setProperty("StringInputWorkspace", data)
            copyData = self.getProperty("StringInputWorkspace").value

            # NOTE:
            # StringArrayProperty cannot be optional, so the background can only be passed in as a string
            # or a list, which will be manually unpacked here
            if bkg != "":
                bkg = [
                    AnalysisDataService.retrieve(me)
                    for me in map(str.strip, bkg.split(","))
                ]

            # NOTE:
            # xMin and xMax are initialized as empty numpy.array (np.array([])).
            _xMin, _xMax = self._locate_global_xlimit()
            xMin = _xMin if xMin.size == 0 else xMin
            xMax = _xMax if xMax.size == 0 else xMax

            # BEGIN_FOR: prcess_spectra
            for n, _wsn in enumerate(copyData):
                _mskn = f"__mask_{n}"  # calculated in previous loop
                _ws = AnalysisDataService.retrieve(_wsn)

                # resample spectra
                _ws_resampled = ResampleX(
                    InputWorkspace=f"__ws_{n}",
                    XMin=xMin,
                    XMax=xMax,
                    NumberBins=numberBins,
                    EnableLogging=False,
                )

                # calibration
                if cal is not None:
                    _ws_cal_resampled = self._resample_calibration(
                        _ws, _mskn, xMin, xMax
                    )
                    _ws_resampled = Divide(
                        LHSWorkspace=_ws_resampled,
                        RHSWorkspace=_ws_cal_resampled,
                        EnableLogging=False,
                    )
                else:
                    _ws_cal_resampled = None

                _ws_resampled = Scale(
                    InputWorkspace=_ws_resampled,
                    Factor=self._get_scale(cal) / self._get_scale(_ws),
                    EnableLogging=False,
                )

                # background
                if bkg != "":
                    bgn = bkg[n] if isinstance(bkg, list) else bkg

                    _ws_bkg_resampled = self._resample_background(
                        bgn, _ws, _mskn, xMin, xMax, _ws_cal_resampled
                    )

                    _ws_resampled = Minus(
                        LHSWorkspace=_ws_resampled,
                        RHSWorkspace=_ws_bkg_resampled,
                        EnableLogging=False,
                    )

                # conjoin
                if n < 1:
                    CloneWorkspace(
                        InputWorkspace=_ws_resampled,
                        OutputWorkspace="__ws_conjoined",
                        EnableLogging=False,
                    )
                else:
                    ConjoinWorkspaces(
                        InputWorkspace1="__ws_conjoined",
                        InputWorkspace2=_ws_resampled,
                        CheckOverlapping=False,
                        EnableLogging=False,
                    )
            # END_FOR: prcess_spectra

            # Step_3: sum all spectra
            # ref: https://docs.mantidproject.org/nightly/algorithms/SumSpectra-v1.html
            if cal is not None:
                SumSpectra(
                    InputWorkspace="__ws_conjoined",
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=False,
                    EnableLogging=False,
                )
            else:
                SumSpectra(
                    InputWorkspace="__ws_conjoined",
                    OutputWorkspace=outWS,
                    WeightedSum=True,
                    MultiplyBySpectra=True,
                    EnableLogging=False,
                )

            self.setProperty("OutputWorkspace", outWS)

            # Step_4: remove temp workspaces
            [
                DeleteWorkspace(ws, EnableLogging=False)
                for ws in self.temp_workspace_list
                if mtd.doesExist(ws)
            ]

    def _get_scale(self, x):
        """return the scale factor needed during normalization"""
        normaliseBy = self.getProperty("NormaliseBy").value

        if x is None:
            return 1
        else:
            if str(normaliseBy).lower() == "none":
                return 1
            elif str(normaliseBy).lower() == "monitor":
                return x.run().getProtonCharge()
            elif str(normaliseBy).lower() == "time":
                return x.run().getLogData("duration").value
            else:
                raise ValueError(f"Unknown normalize type: {normaliseBy}")

    def _locate_global_xlimit(self):
        
        self.declareProperty(
            StringArrayProperty(
                "ArrayInputWorkspace",
                direction=Direction.Input,
                validator=ADSValidator(),
                ),
        )

        """Find the global bin from all spectrum"""
        input_workspaces = self.getProperty("InputWorkspace").value
        mask = self.getProperty("MaskWorkspace").value
        maks_angle = self.getProperty("MaskAngle").value
        target = self.getProperty("Target").value
        e_fixed = self.getProperty("EFixed").value
        outMult = self.getProperty("MultipleOutput").value
        outWS = self.getPropertyValue("OutputWorkspace")
        
        self.setProperty("ArrayInputWorkspace", input_workspaces)
        copyInput = self.getProperty("ArrayInputWorkspace").value

        # NOTE:
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.
        _xMin, _xMax = 1e16, -1e16

        # BEGIN_FOR: located_global_xMin&xMax
        for n, _wsn in enumerate(copyInput):
            _ws = AnalysisDataService.retrieve(_wsn)
            _mskn = f"__mask_{n}"
            self.temp_workspace_list.append(_mskn)

            ExtractMask(_ws, OutputWorkspace=_mskn, EnableLogging=False)
            if maks_angle != Property.EMPTY_DBL:
                MaskAngle(
                    Workspace=_mskn,
                    MinAngle=maks_angle,
                    Angle="Phi",
                    EnableLogging=False,
                )
            if mask is not None:
                BinaryOperateMasks(
                    InputWorkspace1=_mskn,
                    InputWorkspace2=mask,
                    OperationType="OR",
                    OutputWorkspace=_mskn,
                    EnableLogging=False,
                )

            _ws_tmp = ExtractUnmaskedSpectra(
                InputWorkspace=_ws, MaskWorkspace=_mskn, EnableLogging=False
            )
            if isinstance(mtd["_ws_tmp"], IEventWorkspace):
                _ws_tmp = Integration(InputWorkspace=_ws_tmp, EnableLogging=False)
            _ws_tmp = ConvertSpectrumAxis(
                InputWorkspace=_ws_tmp,
                Target=target,
                EFixed=e_fixed,
                EnableLogging=False,
            )

            # ws_tmp = Transpose(
            #    InputWorkspace=_ws_tmp, OutputWorkspace=f"__ws_{n}", EnableLogging=False
            # )

            _ws_tmp = Transpose(
                InputWorkspace=_ws_tmp, OutputWorkspace=f"__ws_{n}", EnableLogging=False
            )

            _xMin = min(_xMin, _ws_tmp.readX(0).min())
            _xMax = max(_xMax, _ws_tmp.readX(0).max())
        # END_FOR: located_global_xMin&xMax

        return _xMin, _xMax

    def _resample_calibration(
        self,
        current_workspace,
        mask_name,
        x_min,
        x_max,
    ):
        """Perform resample on calibration"""
        cal = self.getProperty("CalibrationWorkspace").value
        target = self.getProperty("Target").value
        e_fixed = self.getProperty("EFixed").value
        number_bins = self.getProperty("NumberBins").value

        _ws_cal = ExtractUnmaskedSpectra(
            InputWorkspace=cal, MaskWorkspace=mask_name, EnableLogging=False
        )
        if isinstance(mtd["_ws_cal"], IEventWorkspace):
            _ws_cal = Integration(InputWorkspace=_ws_cal, EnableLogging=False)
        CopyInstrumentParameters(
            InputWorkspace=current_workspace,
            OutputWorkspace=_ws_cal,
            EnableLogging=False,
        )
        _ws_cal = ConvertSpectrumAxis(
            InputWorkspace=_ws_cal,
            Target=target,
            EFixed=e_fixed,
            EnableLogging=False,
        )
        _ws_cal = Transpose(InputWorkspace=_ws_cal, EnableLogging=False)
        return ResampleX(
            InputWorkspace=_ws_cal,
            XMin=x_min,
            XMax=x_max,
            NumberBins=number_bins,
            EnableLogging=False,
        )

    def _resample_background(
        self,
        current_background,
        current_workspace,
        make_name,
        x_min,
        x_max,
        resmapled_calibration,
    ):
        """Perform resample on given background"""
        cal = self.getProperty("CalibrationWorkspace").value
        target = self.getProperty("Target").value
        e_fixed = self.getProperty("EFixed").value
        number_bins = self.getProperty("NumberBins").value

        _ws_bkg = ExtractUnmaskedSpectra(
            InputWorkspace=current_background,
            MaskWorkspace=make_name,
            EnableLogging=False,
        )

        if isinstance(mtd["_ws_bkg"], IEventWorkspace):
            _ws_bkg = Integration(InputWorkspace=_ws_bkg, EnableLogging=False)

        CopyInstrumentParameters(
            InputWorkspace=current_workspace,
            OutputWorkspace=_ws_bkg,
            EnableLogging=False,
        )

        _ws_bkg = ConvertSpectrumAxis(
            InputWorkspace=_ws_bkg,
            Target=target,
            EFixed=e_fixed,
            EnableLogging=False,
        )

        _ws_bkg = Transpose(InputWorkspace=_ws_bkg, EnableLogging=False)

        _ws_bkg_resampled = ResampleX(
            InputWorkspace=_ws_bkg,
            XMin=x_min,
            XMax=x_max,
            NumberBins=number_bins,
            EnableLogging=False,
        )

        if cal is not None:
            _ws_bkg_resampled = Divide(
                LHSWorkspace=_ws_bkg_resampled,
                RHSWorkspace=resmapled_calibration,
                EnableLogging=False,
            )

        _ws_bkg_resampled = Scale(
            InputWorkspace=_ws_bkg_resampled,
            Factor=self._get_scale(cal) / self._get_scale(current_background),
            EnableLogging=False,
        )

        _ws_bkg_resampled = Scale(
            InputWorkspace=_ws_bkg_resampled,
            Factor=self.getProperty("BackgroundScale").value,
            EnableLogging=False,
        )

        return _ws_bkg_resampled

    # def PyExecOld(self):


AlgorithmFactory.subscribe(WANDPowderReduction)
