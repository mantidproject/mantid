#pylint: disable=no-init,invalid-name
"""
System test for CNCS reduction
"""

from mantid.simpleapi import *
import os
import stresstesting

class CNCSReductionTest(stresstesting.MantidStressTest):

    def requiredFiles(self):
        return ["CNCS_51936_event.nxs","CNCS_23936_event.nxs","CNCS_23937_event.nxs"]

    def requiredMemoryMB(self):
        return 4000

    def cleanup(self):
        if os.path.exists(self.groupingFile):
            os.remove(self.groupingFile)
        if os.path.exists(self.parFile):
            os.remove(self.parFile)
        if os.path.exists(self.nxspeFile):
            os.remove(self.nxspeFile)
        if os.path.exists(self.vanFile):
            os.remove(self.vanFile)
        return True


    def runTest(self):
        self.groupingFile=os.path.join(config.getString('defaultsave.directory'),'CNCS_powder_group.xml')
        self.parFile=os.path.join(config.getString('defaultsave.directory'),'CNCS_powder_group.par')
        self.nxspeFile=os.path.join(config.getString('defaultsave.directory'),'CNCS_powder_group.nxspe')
        self.vanFile=os.path.join(config.getString('defaultsave.directory'),'van.nx5')

        config['default.facility']="SNS"
        Load(Filename='CNCS_23936-23937',OutputWorkspace='sum')
        GenerateGroupingPowder(InputWorkspace="sum",AngleStep=0.5,GroupingFilename=self.groupingFile)
        Ei=mtd['sum'].getRun()['EnergyRequest'].firstValue()
        tib=SuggestTibCNCS(Ei)

        DgsReduction(   SampleInputWorkspace="sum",
                        OutputWorkspace="reduced",
                        EnergyTransferRange="-0.2,0.05,2.2",
                        GroupingFile=self.groupingFile,
                        IncidentBeamNormalisation="ByCurrent",
                        TimeIndepBackgroundSub=True,
                        TibTofRangeStart=tib[0],
                        TibTofRangeEnd=tib[1],
                        DetectorVanadiumInputFile="CNCS_51936_event.nxs",
                        UseBoundsForDetVan=True,
                        DetVanIntRangeLow=52000.0,
                        DetVanIntRangeHigh=53000.0,
                        DetVanIntRangeUnits="TOF",
                        SaveProcessedDetVan=True,
                        SaveProcDetVanFilename=self.vanFile)

        rotationdevice="SERotator2"
        psi=mtd["reduced"].run().get(rotationdevice).value[0]
        SaveNXSPE(InputWorkspace="reduced",Filename=self.nxspeFile,Efixed=Ei,psi=psi,KiOverKfScaling=True,ParFile=self.parFile)

    def validate(self):
        #test vanadium file
        self.assertTrue(os.path.exists(self.vanFile))
        van=Load(self.vanFile)
        self.assertEqual(van.blocksize(),1)
        self.assertEqual(van.getNumberHistograms(),51200)
        DeleteWorkspace(van)
        self.assertTrue(os.path.exists(self.nxspeFile))
        LoadNXSPE(self.nxspeFile,OutputWorkspace='nxspe')
        self.disableChecking.append('Instrument')

        return 'nxspe','CNCSReduction_TIBasEvents.nxs'



