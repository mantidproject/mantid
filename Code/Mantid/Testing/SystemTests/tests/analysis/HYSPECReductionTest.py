#pylint: disable=no-init
"""
System test for HYSPEC reduction
"""

from mantid.simpleapi import *
import os
import stresstesting

class HYSPECReductionTest(stresstesting.MantidStressTest):

    def requiredMemoryMB(self):
        return 5000

    def requiredFiles(self):
        return ['HYS_13656_event.nxs','HYS_13657_event.nxs','HYS_13658_event.nxs']

    def cleanup(self):
        if os.path.exists(self.groupingFile):
            os.remove(self.groupingFile)
        return True

    def runTest(self):
        Load(Filename='HYS_13656-13658',OutputWorkspace='sum')
        FilterByLogValue(InputWorkspace='sum',OutputWorkspace='sum1',LogName='s1',MinimumValue='0',MaximumValue='24.5',LogBoundary='Left')
        DeleteWorkspace('sum')
        GenerateEventsFilter(   InputWorkspace='sum1',OutputWorkspace='splboth',InformationWorkspace='info',
                                UnitOfTime='Nanoseconds',LogName='s1',MaximumLogValue='24.5',LogValueInterval='3')
        FilterEvents(   InputWorkspace='sum1',OutputWorkspaceBaseName='split',InformationWorkspace='info',
                        SplitterWorkspace='splboth',FilterByPulseTime='1',GroupWorkspaces='1')
        DeleteWorkspace('split_unfiltered')
        DeleteWorkspace("splboth")
        DeleteWorkspace("info")
        DeleteWorkspace('sum1')
        CompressEvents('split',0.1,OutputWorkspace='splitc')
        DeleteWorkspace('split')
        self.groupingFile=os.path.join(config.getString('defaultsave.directory'),'group4x2.xml')
        GenerateGroupingSNSInelastic(AlongTubes="4",AcrossTubes="2",Instrument="HYSPEC",Filename=self.groupingFile)
        config['default.facility']="SNS"
        DgsReduction(   SampleInputWorkspace='splitc',IncidentBeamNormalisation='ByCurrent',
                        OutputWorkspace='reduced',GroupingFile=self.groupingFile,
                        TimeIndepBackgroundSub ='1',TibTofRangeStart =10400,TibTofRangeEnd =12400,IncidentEnergyGuess=50)
        DeleteWorkspace('splitc')
        SetGoniometer('reduced',Axis0="s1,0,1,0,1")
        SetUB('reduced',5.823,6.475,3.186,90,90,90,'0,1,0','0,0,1')
        ConvertToMD(InputWorkspace='reduced',OutputWorkspace='md',QDimensions='Q3D',
                    QConversionScales='HKL',MinValues='-0.5,-3,-5,-10',MaxValues='0.5,6,2,45')
        DeleteWorkspace('reduced')
        MergeMD(InputWorkspaces='md',OutputWorkspace='merged')
        DeleteWorkspace("md")
        BinMD(  InputWorkspace='merged',AxisAligned='0',BasisVector0='[H,0,0],in 1.079 A^-1,1,0,0,0',
                BasisVector1='[0,K,0],in 0.97 A^-1,0,1,0,0',BasisVector2='[0,0,L],in 1.972 A^-1,0,0,1,0',
                BasisVector3='DeltaE,DeltaE,0,0,0,1',
                OutputExtents='-3,3,-2,6,-4,-1.5,-3,3',OutputBins='1,100,100,1',Parallel='1',OutputWorkspace='slice')
        DeleteWorkspace("merged")
        DeleteWorkspace("PreprocessedDetectorsWS")

    def validate(self):
        self.tolerance = 1e-8
        return 'slice','HYSPECReduction_TIBasEvents.nxs'

