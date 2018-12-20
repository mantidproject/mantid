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
    expected_fit_params={
        'Name': ['Theta', 'ScaleFactor', 'AirSLD', 'BulkSLD', 'Roughness', 'BackGround', 'Resolution',
                 'SLD_Layer0', 'd_Layer0', 'Rough_Layer0', 'Cost function value'],
        'Value': [2.2999999999999998, 1, 0, 0, 0, 7.9488100000033575e-06, 5, 8.2865399998548345e-07,
                  31.315399998680391, 0, 0.90910656437440196],
        'Error': [0, 0, 0, 0, 0, 1.1949723340559929e-07, 0, 2.2338118077681473e-08, 0.9921668942754267, 0, 0]}
    expected_fit_covariance={
        'Name': ['BackGround', 'SLD_Layer0', 'd_Layer0'],
        'BackGround':[100, -64.519943050096259, 60.825620358818924],
        'SLD_Layer0':[-64.519943050096032, 100, -96.330851077988683],
        'd_Layer0': [60.825555558936458, -96.33084065170361, 100]}

    def __init__(self):
        super(INTERReductionTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_result_file, self.runs_file]

    def validate(self):
        return (self.result_workspace, self.reference_result_file)

    def runTest(self):
        setupInstrument()
        Load(self.runs_file, OutputWorkspace=self.runs_workspace)
        workspaces_to_exclude_from_result = AnalysisDataService.Instance().getObjectNames()
        createTransmissionWorkspaces(self.first_transmission_run_names,
                                     self.second_transmission_run_names,
                                     self.transmission_workspace_names)

        testEventDataTimeSlicing(self.event_run_numbers)
        testReductionOfThreeAngleFringedSolidLiquidExample([45222, 45223, 45224])
        testReductionOfTwoAngleAirLiquidExample([44984, 44985])
        testFittingOfReducedData(44990, 44991, self.expected_fit_params, self.expected_fit_covariance)

        removeWorkspaces(workspaces_to_exclude_from_result)
        GroupWorkspaces(InputWorkspaces=AnalysisDataService.Instance().getObjectNames(),
                        OutputWorkspace=self.result_workspace)
        mtd[self.result_workspace].sortByName()

    @staticmethod
    def regenerateRunsFile():
        setupInstrument()
        regenerateRunsFile(INTERReductionTest.first_transmission_run_names +
                           INTERReductionTest.second_transmission_run_names,
                           INTERReductionTest.run_numbers,
                           INTERReductionTest.event_run_numbers)

    @staticmethod
    def regenerateReferenceFileByReducing():
        setupInstrument()
        test = INTERReductionTest()
        test.runTest()
        SaveNexus(InputWorkspace=INTERReductionTest.result_workspace,
                  Filename=INTERReductionTest.reference_result_file)

    @staticmethod
    def regenerateReferenceFileFromDirectory(reference_file_directory):
        setupInstrument()
        regenerateReferenceFile(reference_file_directory, INTERReductionTest.reference_result_file)


def setupInstrument():
    configI = ConfigService.Instance()
    configI.setString("default.instrument", "INTER")
    configI.setString("default.facility", "ISIS")


def removeWorkspaces(to_remove):
    for workspace_name in to_remove:
        AnalysisDataService.Instance().remove(workspace_name)


def workspaceName(file_path):
    return os.path.splitext(os.path.basename(file_path))[0]


def regenerateReferenceFile(reference_file_directory, output_filename):
    '''Generate the reference file from a given folder of output workspaces'''
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


def regenerateRunsFile(transmission_run_names, run_numbers, event_run_numbers):
    '''Generate the test input file from a range of run numbers and transmission runs.'''
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


def createTransmissionWorkspaces(runs1, runs2, output_names):
    '''Create a transmission workspace for each pair of input runs with the given output names'''
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


def eventRef(run_number, angle, start=0, stop=0, DB='TRANS'):
    '''Perform reflectometry reduction on a slice of the given run for the given
    start/stop times'''
    # Filter the input workspace by the given start/stop time (or end time
    # if stop time is not given)
    run_name=str(run_number)
    run_workspace=mtd[run_name]
    if stop==0:
        stoptime=run_workspace.getRun().getLogData('duration').value
    else:
        stoptime=stop
    filter_ws_name=run_name+'_filter'
    FilterByTime(InputWorkspace=run_name, OutputWorkspace=filter_ws_name, StartTime=start, StopTime=stoptime)
    # Calculate the fraction of proton charge in this slice
    filter_workspace=mtd[filter_ws_name]
    slice_proton_charge = filter_workspace.getRun().getLogData('gd_prtn_chrg').value
    total_proton_charge = run_workspace.getRun().getLogData('gd_prtn_chrg').value
    fraction = slice_proton_charge/total_proton_charge
    duration = filter_workspace.getRun().getLogData('duration').value
    print('Fraction:', fraction)
    print('Slice:', slice_proton_charge)
    print('Duration:', duration)
    # Scale monitors by proton charge and add them to the slice workspace
    Scale(InputWorkspace=run_name+'_monitors', Factor=fraction, OutputWorkspace='mon_slice')
    Rebin(InputWorkspace='mon_slice', OutputWorkspace='mon_rebin', Params='0, 100, 100000', PreserveEvents=False)
    slice_name = str(run_number) + '_' + str(start) + '_' + str(stop)
    Rebin(InputWorkspace=filter_ws_name, OutputWorkspace=slice_name, Params='0, 100, 100000', PreserveEvents=False)
    AppendSpectra(InputWorkspace1='mon_rebin', InputWorkspace2=slice_name,
                  OutputWorkspace=slice_name, MergeLogs=False)
    # Reduce this slice
    ReflectometryReductionOneAuto(InputWorkspace=slice_name, FirstTransmissionRun=DB,
                                  OutputWorkspaceBinned=slice_name+'_ref_binned',
                                  OutputWorkspace=slice_name+'_ref',
                                  OutputWorkspaceWavelength=slice_name+'_lam', Debug=True)
    # Delete interim workspaces
    DeleteWorkspace(slice_name+'_lam')
    DeleteWorkspace(slice_name)
    DeleteWorkspace(slice_name+'_ref')
    DeleteWorkspace('mon_slice')
    DeleteWorkspace('mon_rebin')


def stitchedWorkspaceName(run1_number, run2_number):
    '''Gets the name of the stitched workspace based on the two input workspace names'''
    run1_name=str(run1_number)
    run2_name=str(run2_number)
    run2_short_name=run2_name[-2:]
    return run1_name+'_'+run2_short_name


def quickRef(run_numbers=[], trans_workspace_names=[], angles=[]):
    '''Perform reflectometry reduction on each input run, and stitch the
    reduced workspaces together'''
    reduced_runs=''
    for run_index in range(len(run_numbers)):
        # Set up the reduction properties
        run_name=str(run_numbers[run_index])
        properties = {'InputWorkspace': run_name+'.raw',
                      'FirstTransmissionRun': str(trans_workspace_names[run_index]),
                      'OutputWorkspaceBinned': run_name+'_IvsQ_binned',
                      'OutputWorkspace': run_name+'_IvsQ',
                      'OutputWorkspaceWavelength': run_name+'_IvsLam',
                      'Debug':True}
        # Set ThetaIn if the angles are given
        if angles:
            theta=angles[run_index]
            properties['ThetaIn']=theta
            # Special case to set WavelengthMin for a specific angle
            if theta == 0.8:
                properties['WavelengthMin']=2.6
        # Do the reduction
        ReflectometryReductionOneAuto(**properties)
        reduced_runs=reduced_runs+run_name+'_IvsQ_binned'
        if run_index < len(run_numbers)-1:
            reduced_runs=reduced_runs+','
    # Stitch the results
    first_run_name=str(run_numbers[0])
    dqq = NRCalculateSlitResolution(Workspace=first_run_name+'_IvsQ')
    stitched_name=stitchedWorkspaceName(run_numbers[0], run_numbers[-1])
    Stitch1DMany(InputWorkspaces=reduced_runs, OutputWorkspace=stitched_name, Params='-'+str(dqq), ScaleRHSWorkspace=1)


def twoAngleFit(workspace_name, scalefactor, expected_fit_params, expected_fit_covariance):
    '''Perform a fit on the given workspace and compare to the given results'''
    # Scale and fit
    Scale(InputWorkspace=workspace_name, OutputWorkspace=workspace_name+'_scaled', Factor=(1.0/scalefactor))
    function_name='name=ReflectivityMulf, nlayer=1, Theta=2.3, ScaleFactor=1, AirSLD=0, BulkSLD=0, Roughness=0, BackGround=6.8e-06,'\
        'Resolution=5.0, SLD_Layer0=1.0e-6, d_Layer0=20.0, Rough_Layer0=0.0, constraints=(0<SLD_Layer0, 0<d_Layer0),'\
        'ties=(Theta=2.3, AirSLD=0, BulkSLD=0, Resolution=5.0, ScaleFactor=1.0, Roughness=0, Rough_Layer0=0)'
    Fit(Function=function_name,
        InputWorkspace=workspace_name+'_scaled', IgnoreInvalidData='1',
        Output=workspace_name+'_fit', OutputCompositeMembers='1', ConvolveMembers='1')
    # Get output tables
    params_table=mtd[workspace_name+'_fit_Parameters']
    covariance_table=mtd[workspace_name+'_fit_NormalisedCovarianceMatrix']
    # Print output info
    sld=round(params_table.cell(7, 1), 9)
    thick=round(params_table.cell(8, 1), 2)
    dNb=sld*thick
    print('dNb ', dNb)
    print('SLD ', sld)
    print('Thick ', thick)
    print('-----------')
    # Annoyingly, fitting/gsl seems unstable across different platforms so the results don't match
    # accurately. To get around this remove the offending workspaces from the reference and check
    # manually here instead with a more generous tolerance. This isn't ideal but should be enough.
    tolerance=1e-2
    compareFitResults(params_table.toDict(), expected_fit_params, tolerance)
    compareFitResults(covariance_table.toDict(), expected_fit_covariance, tolerance)
    removeWorkspaces([workspace_name+'_fit_Parameters', workspace_name+'_fit_NormalisedCovarianceMatrix'])


def compareFitResults(results_dict, reference_dict, tolerance):
    '''Compare the fit results to the reference. The output table workspaces from the fit
    should be converted to dicts before calling this function'''
    for key in reference_dict:
        if key == 'Name':
            continue

        if key not in results_dict:
            raise ValueError("The column {0} was not found in the fit output table".format(key))

        reference_values = reference_dict[key]
        values_fitted = results_dict[key]
        if len(values_fitted) != len(reference_values):
            raise ValueError("The values fitted and the reference values must be provided as lists (with the same "
                             "number of elements).\nGot actual (length {0}) vs reference (length {1}):\n{2}\n{3}".
                             format(len(values_fitted), len(reference_values), values_fitted, reference_values))

        for index, (value, expected) in enumerate(zip(values_fitted, reference_values)):
            if abs(value - expected) > tolerance:
                logger.error("For the parameter with index {0}, the value found '{1}' differs from "
                             "reference '{2}' by more than required tolerance '{3}'".
                             format(index, value, expected, tolerance))
                logger.error("These were the values found:         {0}".
                             format(values_fitted))
                raise RuntimeError("Some results were not as accurate as expected. Please check the log "
                                   "messages for details")


def generateTimeSlices(run_number):
    '''Generate 60 second time slices of the given run, and perform reflectometry
    reduction on each slice'''
    for slice_index in range(5):
        start=slice_index*60
        stop=(slice_index+1)*60
        eventRef(run_number, 0.5, start, stop, DB='TRANS')


def testEventDataTimeSlicing(event_run_numbers):
    for run_number in event_run_numbers:
        generateTimeSlices(run_number)


def testReductionOfThreeAngleFringedSolidLiquidExample(run_numbers):
    quickRef(run_numbers,['TRANS','TRANS','TRANS'])


def testReductionOfTwoAngleAirLiquidExample(run_numbers):
    quickRef(run_numbers,['TRANS_SM','TRANS_noSM'], angles=[0.8, 2.3])


def testFittingOfReducedData(run1_number, run2_number, expected_fit_params, expected_fit_covariance):
    #D2O run:
    CloneWorkspace(InputWorkspace='44984_85', OutputWorkspace='D2O_IvsQ_binned')
    #fit d2o to get scalefactor
    function_name='name=ReflectivityMulf, nlayer=0, Theta=2.3, ScaleFactor=1.0, AirSLD=0, BulkSLD=6.35e-6, Roughness=2.5,'\
        'BackGround=3.0776e-06, Resolution=5.0, ties=(Theta=2.3, AirSLD=0, BulkSLD=6.35e-6, Resolution=5.0, Roughness=2.5)'
    Fit(Function=function_name,
        InputWorkspace='D2O_IvsQ_binned', IgnoreInvalidData='1', Minimizer='Simplex', Output='D2O_fit',
        OutputCompositeMembers='1', ConvolveMembers='1', StartX='0.0015', EndX='0.3359')
    scalefactor=round(mtd['D2O_fit_Parameters'].cell(1, 1), 3)
    #Create reduced workspace for test:
    quickRef([run1_number, run2_number],['TRANS_SM','TRANS_noSM'], angles=[0.8, 2.3])
    #Test fitting of the result:
    print('run ', str(run1_number))
    stitched_name=stitchedWorkspaceName(run1_number, run2_number)
    twoAngleFit(stitched_name, scalefactor, expected_fit_params, expected_fit_covariance)


# If you want to re-run the test and save the result as a reference...
#   INTERReductionTest.regenerateReferenceFileByReducing()

# or
# If you have workspaces in a folder to use as a reference...
#   INTERReductionTest.regenerateReferenceFileFromDirectory("Path/To/Folder")
