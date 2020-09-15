# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import (ADSValidator, AlgorithmFactory, DataProcessorAlgorithm,
                        IEventWorkspace,
                        MatrixWorkspaceProperty, PropertyMode)
from mantid.dataobjects import MaskWorkspaceProperty
from mantid.simpleapi import (AnalysisDataService, ConvertSpectrumAxis, ConjoinWorkspaces, Transpose,
                              ResampleX, CopyInstrumentParameters, CloneWorkspace,
                              Divide, DeleteWorkspace, Scale,
                              MaskAngle, ExtractMask, Minus, SumSpectra,
                              ExtractUnmaskedSpectra, mtd,
                              BinaryOperateMasks, Integration)
from mantid.kernel import StringListValidator, Direction, Property, FloatBoundedValidator, StringArrayProperty


class WANDPowderReduction(DataProcessorAlgorithm):
    temp_workspace_list = ["__ws_conjoined", "_ws_cal", "_ws_cal_resampled", "_ws_tmp", "_ws_resampled"]

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ['LoadWAND','SaveFocusedXYE']

    def summary(self):
        return 'Performs powder diffraction data reduction for WAND'

    def name(self):
        return "WANDPowderReduction"

    def validateInputs(self):
        issues = dict()
        return issues

    def PyInit(self):

        # self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
        #                                              direction=Direction.Input),
        #                      doc='The main input workspace.')
        self.declareProperty(StringArrayProperty(
                                "InputWorkspace",
                                direction=Direction.Input,
                                validator=ADSValidator(),
                                ),
                            doc='The main input workspace[s].',
                        )

        self.declareProperty(MatrixWorkspaceProperty("BackgroundWorkspace", '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='The background workspace to be subtracted.')
        # self.declareProperty(StringArrayProperty(
        #                         "BackgroundWorkspace",
        #                         # optional=PropertyMode.Optional,
        #                         direction=Direction.Input,
        #                         validator=ADSValidator(),
        #                         ),
        #                      doc='The background workspace[s] to be subtracted.',
        #                 )

        self.declareProperty(MatrixWorkspaceProperty("CalibrationWorkspace", '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='The calibration (vandiaum) workspace.')

        self.declareProperty("BackgroundScale", 1.0,
                             validator=FloatBoundedValidator(0.0),
                             doc="The background will be scaled by this number before being subtracted.")

        self.declareProperty(MaskWorkspaceProperty("MaskWorkspace", '',
                                                   optional=PropertyMode.Optional,
                                                   direction=Direction.Input),
                             doc='The mask from this workspace will be applied before reduction')

        self.copyProperties('ConvertSpectrumAxis', ['Target', 'EFixed'])

        self.copyProperties('ResampleX', ['XMin', 'XMax', 'NumberBins', 'LogBinning'])

        self.declareProperty('NormaliseBy', 'Monitor', StringListValidator(['None', 'Time', 'Monitor']), "Normalise to monitor or time. ")

        self.declareProperty('MaskAngle', Property.EMPTY_DBL,
                             "Phi angle above which will be masked. See :ref:`MaskAngle <algm-MaskAngle>` for details.")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                                                     direction=Direction.Output),
                             doc="Output Workspace")

    def PyExec(self):
        data = self.getProperty("InputWorkspace").value             # [1~n]
        bkg = self.getProperty("BackgroundWorkspace").value         # [1~n]
        cal = self.getProperty("CalibrationWorkspace").value        # [1]
        mask = self.getProperty("MaskWorkspace").value              # [1]
        target = self.getProperty("Target").value
        eFixed = self.getProperty("EFixed").value
        xMin = self.getProperty("XMin").value
        xMax = self.getProperty("XMax").value
        numberBins = self.getProperty("NumberBins").value
        normaliseBy = self.getProperty("NormaliseBy").value
        maskAngle = self.getProperty("MaskAngle").value
        outWS = self.getPropertyValue("OutputWorkspace")

        _get_scale = lambda x : {
                None: 1,
                "Monitor": x.run().getProtonCharge(),
                "Time":    x.run().getLogData("duration").value,
            }[normaliseBy]

        # NOTE:
        # StringArrayProperty cannot be optional, so the background can only be passed in as a string
        # or a list, which will be manually unpacked here
        try:
            bkg = bkg.split(",") if bkg is not None else bkg
        except:
            _bg = AnalysisDataService.retrieve(bkg[0])  # invalid background ws will be detected here

        # NOTE:
        # Due to range difference among incoming spectra, a common bin para is needed
        # such that all data can be binned exactly the same way.

        # Step_1: Find the global bin from all spectrum
        _xMin = 1e16
        _xMax = -1e16
        # BEGIN_FOR: located_global_xMin&xMax
        for n, _wsn in enumerate(data):
            _ws = AnalysisDataService.retrieve(_wsn)
            _mskn = f"__mask_{n}"
            self.temp_workspace_list.append(_mskn)

            ExtractMask(_ws, OutputWorkspace=_mskn, EnableLogging=False)
            if maskAngle != Property.EMPTY_DBL:
                MaskAngle(Workspace=_mskn, MinAngle=maskAngle, Angle="Phi", EnableLogging=False)
            if mask is not None:
                BinaryOperateMasks(
                    InputWorkspace1=_mskn, 
                    InputWorkspace2=mask,
                    OperationType="OR", 
                    OutputWorkspace=_mskn, 
                    EnableLogging=False,
                    )
            
            _ws_tmp = Scale(InputWorkspace=_wsn, Factor=_get_scale(cal)/_get_scale(_ws), EnableLogging=False)
            _ws_tmp = ExtractUnmaskedSpectra(InputWorkspace=_ws_tmp, MaskWorkspace=_mskn, EnableLogging=False)
            _ws_tmp = Integration(InputWorkspace=_ws_tmp, EnableLogging=False)
            _ws_tmp = ConvertSpectrumAxis(InputWorkspace=_ws_tmp, Target=target, EFixed=eFixed, EnableLogging=False)
            _ws_tmp = Transpose(InputWorkspace=_ws_tmp, OutputWorkspace=f"__ws_{n}", EnableLogging=False)

            _xMin = min(_xMin, _ws_tmp.readX(0).min())
            _xMax = max(_xMax, _ws_tmp.readX(0).max())
        # END_FOR: located_global_xMin&xMax

        # NOTE:
        # xMin and xMax are initialized as empty numpy.array (np.array([])).
        xMin = _xMin if xMin.size == 0 else xMin
        xMax = _xMax if xMax.size == 0 else xMax

            _ws = AnalysisDataService.retrieve(f"__data_tmp_{n}")

            _xMin, _xMax = min(_xMin, _ws.readX(0).min()), max(_xMax, _ws.readX(0).max())

        xMin = _xMin if xMin is None else xMin
        xMax = _xMax if xMax is None else xMax

        _data_tmp_list = [me for me in self.temp_workspace_list if '__data_tmp_' in me]
        for i, _wsn in enumerate(_data_tmp_list):
            ResampleX(InputWorkspace=_wsn, OutputWorkspace=_wsn, XMin=xMin, XMax=xMax, NumberBins=numberBins, EnableLogging=False)

        if bkg is not None:
            bkg = bkg.split(",")  # need to do a manual splitting here since StringArrayProperty can't be optional
            for n, bgn in enumerate(bkg):
                self.temp_workspace_list.append(f'__bkg_tmp_{n}')
                ExtractUnmaskedSpectra(InputWorkspace=bkg, MaskWorkspace='__mask_tmp', OutputWorkspace=f'__bkg_tmp_{n}', EnableLogging=False)
                if isinstance(mtd[f'__bkg_tmp_{n}'], IEventWorkspace):
                    Integration(InputWorkspace=f'__bkg_tmp_{n}', OutputWorkspace=f'__bkg_tmp_{n}', EnableLogging=False)
                CopyInstrumentParameters('__data_tmp_0', f'__bkg_tmp_{n}', EnableLogging=False)
                ConvertSpectrumAxis(InputWorkspace=f'__bkg_tmp_{n}', Target=target, EFixed=eFixed, OutputWorkspace=f'__bkg_tmp_{n}', EnableLogging=False)
                Transpose(InputWorkspace=f'__bkg_tmp_{n}', OutputWorkspace=f'__bkg_tmp_{n}', EnableLogging=False)
                ResampleX(InputWorkspace=f'__bkg_tmp_{n}', OutputWorkspace=f'__bkg_tmp_{n}', XMin=xMin, XMax=xMax, NumberBins=numberBins, EnableLogging=False)
                Scale(InputWorkspace=f'__bkg_tmp_{n}', OutputWorkspace=f'__bkg_tmp_{n}', Factor=_get_scale(cal)/_get_scale(bkg), EnableLogging=False)
                Scale(InputWorkspace=f'__bkg_tmp_{n}', OutputWorkspace=f'__bkg_tmp_{n}', Factor=self.getProperty('BackgroundScale').value, EnableLogging=False)

        if cal is not None:
            ExtractUnmaskedSpectra(InputWorkspace=cal, MaskWorkspace='__mask_tmp', OutputWorkspace='__cal_tmp', EnableLogging=False)
            if isinstance(mtd['__cal_tmp'], IEventWorkspace):
                Integration(InputWorkspace='__cal_tmp', OutputWorkspace='__cal_tmp', EnableLogging=False)
            CopyInstrumentParameters('__data_tmp_0', '__cal_tmp', EnableLogging=False)
            ConvertSpectrumAxis(InputWorkspace='__cal_tmp', Target=target, EFixed=eFixed, OutputWorkspace='__cal_tmp', EnableLogging=False)
            Transpose(InputWorkspace='__cal_tmp', OutputWorkspace='__cal_tmp', EnableLogging=False)
            ResampleX(InputWorkspace='__cal_tmp', OutputWorkspace='__cal_tmp', XMin=xMin, XMax=xMax, NumberBins=numberBins, EnableLogging=False)

            Divide(LHSWorkspace='__data_tmp_0', RHSWorkspace='__cal_tmp', OutputWorkspace=outWS, EnableLogging=False)
            if bkg is not None:
                Minus(LHSWorkspace=outWS, RHSWorkspace='__bkg_tmp_0', OutputWorkspace=outWS, EnableLogging=False)
        else:
            CloneWorkspace(InputWorkspace="__data_tmp_0", OutputWorkspace=outWS)

        for i, _wsn in enumerate(_data_tmp_list[1:]):
            if cal is not None:
                Divide(LHSWorkspace=_wsn, RHSWorkspace='__cal_tmp', OutputWorkspace=_wsn, EnableLogging=False)
            if bkg is not None:
                Minus(LHSWorkspace=_wsn, RHSWorkspace='__bkg_tmp_0', OutputWorkspace=_wsn, EnableLogging=False)
            ConjoinWorkspaces(InputWorkspace1=outWS,
                              InputWorkspace2=_wsn,
                              CheckOverlapping=False,
                            )

        SumSpectra(InputWorkspace=outWS, OutputWorkspace=outWS, WeightedSum=True, MultiplyBySpectra=False, StoreInADS=False)

        self.setProperty("OutputWorkspace", outWS)

        # remove temp workspaces
        [DeleteWorkspace(ws, EnableLogging=False) for ws in self.temp_workspace_list if mtd.doesExist(ws)]


AlgorithmFactory.subscribe(WANDPowderReduction)
