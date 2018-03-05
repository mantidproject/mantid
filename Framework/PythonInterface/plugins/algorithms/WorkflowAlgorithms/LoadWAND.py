from __future__ import absolute_import, division, print_function
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, FileProperty, FileAction, WorkspaceProperty
from mantid.kernel import Direction, UnitConversion, Elastic
from mantid.simpleapi import LoadEventNexus, Integration, mtd, SetGoniometer, DeleteWorkspace, AddSampleLog, MaskBTP
from six.moves import range


class LoadWAND(DataProcessorAlgorithm):
    def name(self):
        return 'LoadWAND'

    def category(self):
        return 'DataHandling\\Nexus'

    def summary(self):
        return 'Loads Event Nexus file, integrates events, sets wavelength, mask, and goniometer, and sets proton charge to monitor counts'

    def PyInit(self):
        self.declareProperty(FileProperty(name="Filename", defaultValue="", action=FileAction.Load, extensions=[".nxs.h5"]))
        self.declareProperty("Wavelength", 1.488, doc="Wavelength to set the workspace")
        self.declareProperty("ApplyMask", True, "If True standard masking will be applied to the workspace")
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def PyExec(self):
        filename = self.getProperty("Filename").value
        wavelength = self.getProperty("wavelength").value
        outWS = self.getPropertyValue("OutputWorkspace")

        LoadEventNexus(Filename=filename, OutputWorkspace=outWS, LoadMonitors=True)
        Integration(InputWorkspace=outWS, OutputWorkspace=outWS)

        if self.getProperty("ApplyMask").value:
            MaskBTP(outWS, Bank='8', Tube='475-480')
            MaskBTP(outWS, Pixel='1,2,511,512')

        mtd[outWS].getAxis(0).setUnit("Wavelength")
        w = [wavelength-0.001, wavelength+0.001]
        for idx in range(mtd[outWS].getNumberHistograms()):
            mtd[outWS].setX(idx, w)

        SetGoniometer(outWS, Axis0="HB2C:Mot:s1,0,1,0,1")
        AddSampleLog(outWS, LogName="gd_prtn_chrg", LogType='Number', NumberType='Double',
                     LogText=str(mtd[outWS+'_monitors'].getNumberEvents()))
        DeleteWorkspace(outWS+'_monitors')

        AddSampleLog(outWS, LogName="Wavelength", LogType='Number', NumberType='Double', LogText=str(wavelength))
        AddSampleLog(outWS, LogName="Ei", LogType='Number', NumberType='Double',
                     LogText=str(UnitConversion.run('Wavelength', 'Energy', wavelength, 0, 0, 0, Elastic, 0)))

        self.setProperty('OutputWorkspace', outWS)


AlgorithmFactory.subscribe(LoadWAND)
