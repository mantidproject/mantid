# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry reduction
Adapted from scripts provided by Max Skoda.
"""
from __future__ import (print_function)
import systemtesting
from mantid.simpleapi import *
from mantid import ConfigService


class INTERReductionTest(systemtesting.MantidSystemTest):
    '''
    Mantid Test Script for INTER:

    Tests:
    1. Event data time-slicing
    2. "Quickref" - used when the autoreduction unwantedly sums runs together.
    3. Scripted fitting of reduced data for NRW
    4. Linear detector reduction
    '''
    # Note: you may find the regenerate functions useful.

    event_run_numbers = [45222]
    run_numbers = [45222, 45223, 45224, 44984, 44985, 44990, 44991]
    first_transmission_run_names = ['45226', '44988', '44986']
    second_transmission_run_names = ['45227', '44989', '44987']
    transmission_workspace_names = ['TRANS', 'TRANS_SM', 'TRANS_NoSM']
    runs_file = 'INTERReductionTestRuns.nxs'
    runs_workspace = 'Runs'
    reference_result_file = 'INTERReductionResult.nxs'
    result_workspace = 'Result'

    def __init__(self):
        super(INTERReductionTest, self).__init__()
        self.tolerance = 0.00000001

    def requiredFiles(self):
        return [self.reference_result_file, self.runs_file]

    def validate(self):
        return (self.result_workspace, self.reference_result_file)

    def runTest(self):
        SetupInstrument()
        Load(self.runs_file, OutputWorkspace=self.runs_workspace)
        workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()
        CreateTransmissionWorkspaces(self.first_transmission_run_names,
                                     self.second_transmission_run_names,
                                     self.transmission_workspace_names)

        TestEventDataTimeSlicing(self.event_run_numbers)
        TestReductionOfThreeAngleFringedSolidLiquidExample()
        TestReductionOfTwoAngleAirLiquidExample()
        TestFittingOfReducedData()

        RemoveWorkspaces(workspaces_to_exclude_from_result)
        GroupWorkspaces(InputWorkspaces=AnalysisDataService.Instance().getObjectNames(),
                        OutputWorkspace=self.result_workspace)
        mtd[self.result_workspace].sortByName()

    @staticmethod
    def regenerateRunsFile():
        SetupInstrument()
        RegenerateRunsFile(INTERReductionTest.first_transmission_run_names +
                           INTERReductionTest.second_transmission_run_names,
                           INTERReductionTest.run_numbers,
                           INTERReductionTest.event_run_numbers)

    @staticmethod
    def regenerateReferenceFileByReducing():
        SetupInstrument()
        test = INTERReductionTest()
        test.runTest()
        SaveNexus(InputWorkspace=INTERReductionTest.result_workspace,
                  Filename=INTERReductionTest.reference_result_file)

    @staticmethod
    def regenerateReferenceFileFromDirectory(reference_file_directory):
        SetupInstrument()
        RegenerateReferenceFile(reference_file_directory, INTERReductionTest.reference_result_file)


def SetupInstrument():
    configI = ConfigService.Instance()
    configI.setString("default.instrument", "INTER")
    configI.setString("default.facility", "ISIS")


def RemoveWorkspaces(to_remove):
    for workspace_name in to_remove:
        AnalysisDataService.Instance().remove(workspace_name)


def WorkspaceName(file_path):
    return os.path.splitext(os.path.basename(file_path))[0]


def RegenerateReferenceFile(reference_file_directory, output_filename):
    files = os.listdir(reference_file_directory)
    workspace_names = []
    for file in files:
        workspace_name = WorkspaceName(file)
        Load(file, OutputWorkspace=workspace_name)
        workspace_names.append(workspace_name)

    output_workspace_name = 'Output'
    GroupWorkspaces(InputWorkspaces=workspace_names, OutputWorkspace=output_workspace_name)
    mtd[output_workspace_name].sortByName()
    SaveNexus(InputWorkspace=output_workspace_name, Filename=output_filename)


def RegenerateRunsFile(transmission_run_names, run_numbers, event_run_numbers):
    "This is used to generate the test input file from a range of run numbers"
    "and transmission runs."
    # Load transmission runs
    for run in transmission_run_names:
        Load('{}.raw'.format(run), OutputWorkspace=run)
    # Load raw run files
    run_names = [str(run_number)+'.raw' for run_number in run_numbers]
    for run_name in run_names:
        Load(run_name, OutputWorkspace=run_name)
    # Load event workspaces
    event_run_names = [str(event_run_number) for event_run_number in event_run_numbers]
    for event_run_name in event_run_names:
        LoadEventNexus(event_run_name, OutputWorkspace=event_run_name, LoadMonitors=True)
    event_monitor_names = [str(run_number)+'_monitors' for run_number in event_run_numbers]
    # Group and save
    GroupWorkspaces(InputWorkspaces=run_names + transmission_run_names + event_run_names +
                    event_monitor_names,
                    OutputWorkspace='Input')
    SaveNexus(InputWorkspace='Input', Filename='INTERReductionTestRuns.nxs')


def CreateTransmissionWorkspaces(runs1, runs2, output_names):
    for run1, run2, name in zip(runs1, runs2, output_names):
        CreateTransmissionWorkspaceAuto(
            FirstTransmissionRun=run1,
            SecondTransmissionRun=run2,
            OutputWorkspace=name,
            StartOverlap=10,
            EndOverlap=12)
        # Delete debug workspaces
        DeleteWorkspace('TRANS_LAM_'+run1)
        DeleteWorkspace('TRANS_LAM_'+run2)


def EventRef(runno,angle,stop=0,start=0,DB='TRANS'):
    '''Event data time-slicing'''
    runno=str(runno)
    w1=mtd[runno]
    total = w1.getRun().getLogData('gd_prtn_chrg').value
    end = w1.getRun().getLogData('duration').value
    if stop==0:
        stoptime=end
    else:
        stoptime=stop
    FilterByTime(InputWorkspace=runno, OutputWorkspace=runno+'_filter', StartTime=start, StopTime=stoptime)
    wt=mtd[runno+'_filter']
    slice = wt.getRun().getLogData('gd_prtn_chrg').value
    fraction = slice/total
    duration = wt.getRun().getLogData('duration').value
    print('Fraction:',fraction)
    print('Slice:',slice)
    print('Duration:',duration)

    Scale(InputWorkspace=runno+'_monitors',Factor=fraction,OutputWorkspace='mon_slice')
    Rebin(InputWorkspace=runno+'_filter', OutputWorkspace=runno+'_'+str(start)+'_'+str(stop), Params='0,100,100000', PreserveEvents=False)
    Rebin(InputWorkspace='mon_slice', OutputWorkspace='mon_rebin', Params='0,100,100000', PreserveEvents=False)
    AppendSpectra(InputWorkspace1='mon_rebin', InputWorkspace2=runno+'_'+str(start)+'_'+str(stop),
                  OutputWorkspace=runno+'_'+str(start)+'_'+str(stop), MergeLogs=False)
    ReflectometryReductionOneAuto(InputWorkspace=runno+'_'+str(start)+'_'+str(stop), FirstTransmissionRun=DB,
                                  OutputWorkspaceBinned=runno+'_'+str(start)+'_'+str(stop)+'_ref_binned',
                                  OutputWorkspace=runno+'_'+str(start)+'_'+str(stop)+'_ref',
                                  OutputWorkspaceWavelength=runno+'_'+str(start)+'_'+str(stop)+'_lam',Debug=True)


def QuickRef(runs=[], trans=[], angles=[]):
    '''Use of "QuickRef" - scripted reduction'''
    list=''
    if not angles:
        for i in range(len(runs)):
            run1=runs[i]
            trans1=trans[i]
            ReflectometryReductionOneAuto(InputWorkspace=str(run1)+'.raw', FirstTransmissionRun=str(trans1),
                                          OutputWorkspaceBinned=str(run1)+'_IvsQ_binned', OutputWorkspace=str(run1)+'_IvsQ',
                                          OutputWorkspaceWavelength=str(run1)+'_IvsLam',Debug=True)
            list=list+str(run1)+'_IvsQ_binned'+','
    else:
        for i in range(len(runs)):
            run1=runs[i]
            trans1=trans[i]
            theta=angles[i]
            if theta == 0.8:
                ReflectometryReductionOneAuto(InputWorkspace=str(run1)+'.raw', FirstTransmissionRun=str(trans1),
                                              OutputWorkspaceBinned=str(run1)+'_IvsQ_binned', OutputWorkspace=str(run1)+'_IvsQ',
                                              OutputWorkspaceWavelength=str(run1)+'_IvsLam', ThetaIn=theta, WavelengthMin=2.6,Debug=True)
            else:
                ReflectometryReductionOneAuto(InputWorkspace=str(run1)+'.raw', FirstTransmissionRun=str(trans1),
                                              OutputWorkspaceBinned=str(run1)+'_IvsQ_binned', OutputWorkspace=str(run1)+'_IvsQ',
                                              OutputWorkspaceWavelength=str(run1)+'_IvsLam', ThetaIn=theta,Debug=True)
            if i == len(runs)-1:
                list=list+str(run1)+'_IvsQ_binned'
            else:
                list=list+str(run1)+'_IvsQ_binned'+','
    runno2=str(runs[-1])
    runno2=runno2[-2:]
    dqq = NRCalculateSlitResolution(Workspace=str(runs[0])+'_IvsQ')
    Stitch1DMany(InputWorkspaces=list, OutputWorkspace=str(runs[0])+'_'+str(runno2), Params='-'+str(dqq), ScaleRHSWorkspace=1)


def twoangfit(run1,run2,scalefactor):
    runno=str(run2)
    runno=runno[-2:]
    Scale(InputWorkspace=str(run1)+'_'+str(runno), OutputWorkspace=str(run1)+'_'+str(runno)+'_scaled', Factor=(1.0/scalefactor))
    function_name='name=ReflectivityMulf,nlayer=1,Theta=2.3,ScaleFactor=1,AirSLD=0,BulkSLD=0,Roughness=0,BackGround=6.8e-06,'\
        'Resolution=5.0,SLD_Layer0=1.0e-6,d_Layer0=20.0,Rough_Layer0=0.0,constraints=(0<SLD_Layer0,0<d_Layer0),'\
        'ties=(Theta=2.3,AirSLD=0,BulkSLD=0,Resolution=5.0,ScaleFactor=1.0,Roughness=0,Rough_Layer0=0)'
    Fit(Function=function_name,
        InputWorkspace=str(run1)+'_'+str(runno)+'_scaled',IgnoreInvalidData='1',
        Output=str(run1)+'_'+str(runno)+'_fit',OutputCompositeMembers='1',ConvolveMembers='1')
    sld=round(mtd[str(run1)+'_'+str(runno)+'_fit_Parameters'].cell(7,1),9)
    thick=round(mtd[str(run1)+'_'+str(runno)+'_fit_Parameters'].cell(8,1),2)
    dNb=sld*thick
    print('run ',str(run1))
    print('dNb ',dNb)
    print('SLD ',sld)
    print('Thick ',thick)
    print('-----------')


def GenerateTimeSlices(run):
    '''Generate 60sec time slices'''
    grouped=''
    for ii in range(5):
        slice_name = str(run) + '_' + str(60*ii) + '_' + str(60*(ii+1))
        EventRef(run,0.5,(ii+1)*60,ii*60,DB='TRANS')
        grouped=grouped+', ' + slice_name + '_ref_binned'
        DeleteWorkspace(slice_name+'_lam')
        DeleteWorkspace(slice_name)
        DeleteWorkspace(slice_name+'_ref')
        DeleteWorkspace('mon_slice')
        DeleteWorkspace('mon_rebin')


def TestEventDataTimeSlicing(event_run_numbers):
    for run in event_run_numbers:
        GenerateTimeSlices(run)


def TestReductionOfThreeAngleFringedSolidLiquidExample():
    QuickRef([45222,45223,45224],['TRANS','TRANS','TRANS'])


def TestReductionOfTwoAngleAirLiquidExample():
    QuickRef([44984,44985],['TRANS_SM','TRANS_noSM'], angles=[0.8,2.3])


def TestFittingOfReducedData():
    #D2O run:
    CloneWorkspace(InputWorkspace='44984_85', OutputWorkspace='D2O_IvsQ_binned')
    #fit d2o to get scalefactor
    function_name='name=ReflectivityMulf,nlayer=0,Theta=2.3,ScaleFactor=1.0,AirSLD=0,BulkSLD=6.35e-6,Roughness=2.5,'\
        'BackGround=3.0776e-06,Resolution=5.0,ties=(Theta=2.3,AirSLD=0,BulkSLD=6.35e-6,Resolution=5.0,Roughness=2.5)'
    Fit(Function=function_name,
        InputWorkspace='D2O_IvsQ_binned',IgnoreInvalidData='1',Minimizer='Simplex',Output='D2O_fit',
        OutputCompositeMembers='1',ConvolveMembers='1',StartX='0.0015',EndX='0.3359')
    scalefactor=round(mtd['D2O_fit_Parameters'].cell(1,1),3)
    #Create reduced workspace for test:
    QuickRef([44990,44991],['TRANS_SM','TRANS_noSM'], angles=[0.8,2.3])
    #Test fitting:
    twoangfit(44990,44991,scalefactor)


# If you want to re-run the test and save the result as a reference...
#   INTERReductionTest.regenerateReferenceFileByReducing()

# or
# If you have workspaces in a folder to use as a reference...
#   INTERReductionTest.regenerateReferenceFileFromDirectory("Path/To/Folder")
