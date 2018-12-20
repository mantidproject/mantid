from __future__ import absolute_import, division, print_function
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory,
                        MatrixWorkspaceProperty, PropertyMode)
from mantid.dataobjects import MaskWorkspaceProperty
from mantid.simpleapi import (ConvertSpectrumAxis, Transpose,
                              ResampleX, CopyInstrumentParameters,
                              Divide, DeleteWorkspace, Scale,
                              MaskAngle, ExtractMask, Minus,
                              ExtractUnmaskedSpectra, mtd,
                              BinaryOperateMasks)
from mantid.kernel import StringListValidator, Direction, Property, FloatBoundedValidator


class WANDPowderReduction(DataProcessorAlgorithm):
    temp_workspace_list = ['__data_tmp', '__cal_tmp', '__mask_tmp', '__bkg_tmp']

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

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", '',
                                                     direction=Direction.Input),
                             doc='The main input workspace.')

        self.declareProperty(MatrixWorkspaceProperty("CalibrationWorkspace", '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='The calibration (vandiaum) workspace.')

        self.declareProperty(MatrixWorkspaceProperty("BackgroundWorkspace", '',
                                                     optional=PropertyMode.Optional,
                                                     direction=Direction.Input),
                             doc='The background workspace to be subtracted.')

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

        if normaliseBy == "Monitor":
            data_scale = data.run().getProtonCharge()
        elif normaliseBy == "Time":
            data_scale = data.run().getLogData('duration').value

        ExtractMask(data, OutputWorkspace='__mask_tmp', EnableLogging=False)

        if maskAngle != Property.EMPTY_DBL:
            MaskAngle(Workspace='__mask_tmp', MinAngle=maskAngle, Angle='Phi', EnableLogging=False)

        if mask is not None:
            BinaryOperateMasks(InputWorkspace1='__mask_tmp', InputWorkspace2=mask,
                               OperationType='OR', OutputWorkspace='__mask_tmp', EnableLogging=False)

        ExtractUnmaskedSpectra(InputWorkspace=data, MaskWorkspace='__mask_tmp', OutputWorkspace='__data_tmp', EnableLogging=False)
        ConvertSpectrumAxis(InputWorkspace='__data_tmp', Target=target, EFixed=eFixed, OutputWorkspace=outWS, EnableLogging=False)
        Transpose(InputWorkspace=outWS, OutputWorkspace=outWS, EnableLogging=False)
        ResampleX(InputWorkspace=outWS, OutputWorkspace=outWS, XMin=xMin, XMax=xMax, NumberBins=numberBins, EnableLogging=False)

        if cal is not None:
            ExtractUnmaskedSpectra(InputWorkspace=cal, MaskWorkspace='__mask_tmp', OutputWorkspace='__cal_tmp', EnableLogging=False)
            CopyInstrumentParameters(data, '__cal_tmp', EnableLogging=False)
            ConvertSpectrumAxis(InputWorkspace='__cal_tmp', Target=target, EFixed=eFixed, OutputWorkspace='__cal_tmp', EnableLogging=False)
            Transpose(InputWorkspace='__cal_tmp', OutputWorkspace='__cal_tmp', EnableLogging=False)
            ResampleX(InputWorkspace='__cal_tmp', OutputWorkspace='__cal_tmp', XMin=xMin, XMax=xMax, NumberBins=numberBins,
                      EnableLogging=False)
            Divide(LHSWorkspace=outWS, RHSWorkspace='__cal_tmp', OutputWorkspace=outWS, EnableLogging=False)
            if normaliseBy == "Monitor":
                cal_scale = cal.run().getProtonCharge()
            elif normaliseBy == "Time":
                cal_scale = cal.run().getLogData('duration').value

        Scale(InputWorkspace=outWS, OutputWorkspace=outWS, Factor=cal_scale/data_scale, EnableLogging=False)

        if bkg is not None:
            ExtractUnmaskedSpectra(InputWorkspace=bkg, MaskWorkspace='__mask_tmp', OutputWorkspace='__bkg_tmp', EnableLogging=False)
            CopyInstrumentParameters(data, '__bkg_tmp', EnableLogging=False)
            ConvertSpectrumAxis(InputWorkspace='__bkg_tmp', Target=target, EFixed=eFixed, OutputWorkspace='__bkg_tmp', EnableLogging=False)
            Transpose(InputWorkspace='__bkg_tmp', OutputWorkspace='__bkg_tmp', EnableLogging=False)
            ResampleX(InputWorkspace='__bkg_tmp', OutputWorkspace='__bkg_tmp', XMin=xMin, XMax=xMax, NumberBins=numberBins,
                      EnableLogging=False)
            if cal is not None:
                Divide(LHSWorkspace='__bkg_tmp', RHSWorkspace='__cal_tmp', OutputWorkspace='__bkg_tmp', EnableLogging=False)
            if normaliseBy == "Monitor":
                bkg_scale = bkg.run().getProtonCharge()
            elif normaliseBy == "Time":
                bkg_scale = bkg.run().getLogData('duration').value
            Scale(InputWorkspace='__bkg_tmp', OutputWorkspace='__bkg_tmp', Factor=cal_scale/bkg_scale, EnableLogging=False)
            Scale(InputWorkspace='__bkg_tmp', OutputWorkspace='__bkg_tmp',
                  Factor=self.getProperty('BackgroundScale').value, EnableLogging=False)
            Minus(LHSWorkspace=outWS, RHSWorkspace='__bkg_tmp', OutputWorkspace=outWS, EnableLogging=False)

        self.setProperty("OutputWorkspace", outWS)

        # remove temp workspaces
        [DeleteWorkspace(ws, EnableLogging=False) for ws in self.temp_workspace_list if mtd.doesExist(ws)]


AlgorithmFactory.subscribe(WANDPowderReduction)
