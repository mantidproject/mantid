# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
System Test for ISIS Reflectometry reduction
Adapted from scripts provided by Max Skoda.
"""

from ISISReflectometryWorkflowBase import (
    ISISReflectometryWorkflowBase,
    removeWorkspaces,
    setupInstrument,
    stitchTransmissionWorkspaces,
    stitchedWorkspaceName,
)
from mantid.api import mtd
from mantid.kernel import logger
from mantid.simpleapi import (
    AppendSpectra,
    CloneWorkspace,
    DeleteWorkspace,
    FilterByTime,
    Fit,
    NRCalculateSlitResolution,
    SaveNexus,
    Scale,
    Stitch1DMany,
    Rebin,
    ReflectometryISISLoadAndProcess,
)
import systemtesting


class INTERReductionTest(systemtesting.MantidSystemTest, ISISReflectometryWorkflowBase):
    """
    Mantid Test Script for INTER:

    Tests:
    1. Event data time-slicing
    2. "Quickref" - used when the autoreduction unwantedly sums runs together.
    3. Scripted fitting of reduced data for NRW
    4. Linear detector reduction
    """

    run_numbers = [45222, 45223, 45224, 44984, 44985, 44990, 44991]
    event_run_numbers = [45222]
    first_transmission_runs = ["45226", "44988", "44986"]
    second_transmission_runs = ["45227", "44989", "44987"]
    transmission_workspace_names = ["TRANS", "TRANS_SM", "TRANS_NoSM"]
    input_workspaces_file = "INTERReductionTestRuns.nxs"
    reference_file = "INTERReductionResult.nxs"

    expected_fit_params = {
        "Name": [
            "Theta",
            "ScaleFactor",
            "AirSLD",
            "BulkSLD",
            "Roughness",
            "BackGround",
            "Resolution",
            "SLD_Layer0",
            "d_Layer0",
            "Rough_Layer0",
            "Cost function value",
        ],
        "Value": [
            2.2999999999999998,
            1,
            0,
            0,
            0,
            7.9488100000033575e-06,
            5,
            8.2865399998548345e-07,
            31.315399998680391,
            0,
            0.90910656437440196,
        ],
        "Error": [0, 0, 0, 0, 0, 1.1949723340559929e-07, 0, 2.2338118077681473e-08, 0.9921668942754267, 0, 0],
    }
    expected_fit_covariance = {
        "Name": ["BackGround", "SLD_Layer0", "d_Layer0"],
        "BackGround": [100, -64.519943050096259, 60.825620358818924],
        "SLD_Layer0": [-64.519943050096032, 100, -96.330851077988683],
        "d_Layer0": [60.825555558936458, -96.33084065170361, 100],
    }

    def __init__(self):
        super(INTERReductionTest, self).__init__()
        self.tolerance = 1e-6

    def requiredFiles(self):
        return [self.reference_file, self.input_workspaces_file]

    def validate(self):
        return (self.result_workspace_name, self.reference_file)

    def runTest(self):
        self.setupTest()
        stitchTransmissionWorkspaces(self.first_transmission_runs, self.second_transmission_runs, self.transmission_workspace_names)
        testEventDataTimeSlicing(self.event_run_numbers)
        testReductionOfThreeAngleFringedSolidLiquidExample([45222, 45223, 45224])
        testReductionOfTwoAngleAirLiquidExample([44984, 44985])
        testFittingOfReducedData(44990, 44991, self.expected_fit_params, self.expected_fit_covariance)
        self.finaliseResults()

    def regenerateReferenceFileByReducing(self):
        setupInstrument()
        test = INTERReductionTest()
        test.runTest()
        SaveNexus(InputWorkspace=self.reference_workspace_name, Filename=self.reference_file)


def eventRef(run_number, angle, start=0, stop=0, DB="TRANS"):
    """Perform reflectometry reduction on a slice of the given run for the given
    start/stop times"""
    # Filter the input workspace by the given start/stop time (or end time
    # if stop time is not given)
    run_name = str(run_number)
    run_workspace = mtd[run_name]
    if stop == 0:
        stoptime = run_workspace.getRun().getLogData("duration").value
    else:
        stoptime = stop
    filter_ws_name = run_name + "_filter"
    FilterByTime(InputWorkspace=run_name, OutputWorkspace=filter_ws_name, StartTime=start, StopTime=stoptime)
    # Calculate the fraction of proton charge in this slice
    filter_workspace = mtd[filter_ws_name]
    slice_proton_charge = filter_workspace.getRun().getLogData("gd_prtn_chrg").value
    total_proton_charge = run_workspace.getRun().getLogData("gd_prtn_chrg").value
    fraction = slice_proton_charge / total_proton_charge
    duration = filter_workspace.getRun().getLogData("duration").value
    print("Fraction:", fraction)
    print("Slice:", slice_proton_charge)
    print("Duration:", duration)
    # Scale monitors by proton charge and add them to the slice workspace
    Scale(InputWorkspace=run_name + "_monitors", Factor=fraction, OutputWorkspace="mon_slice")
    # set the duration to be the wrong thing to make comparisons work
    filter_workspace.getRun()["duration"] = run_workspace.getRun().getLogData("duration").value

    Rebin(InputWorkspace="mon_slice", OutputWorkspace="mon_rebin", Params="0, 100, 100000", PreserveEvents=False)
    slice_name = str(run_number) + "_" + str(start) + "_" + str(stop)
    Rebin(InputWorkspace=filter_ws_name, OutputWorkspace=slice_name, Params="0, 100, 100000", PreserveEvents=False)
    AppendSpectra(InputWorkspace1="mon_rebin", InputWorkspace2=slice_name, OutputWorkspace=slice_name, MergeLogs=False)
    # Reduce this slice
    ReflectometryISISLoadAndProcess(
        InputRunList=slice_name,
        FirstTransmissionRunList=DB,
        OutputWorkspaceBinned=slice_name + "_ref_binned",
        OutputWorkspace=slice_name + "_ref",
        OutputWorkspaceWavelength=slice_name + "_lam",
        OutputWorkspaceTransmission=DB + "_LAM",
        Debug=True,
    )
    # Delete interim workspaces
    DeleteWorkspace(slice_name + "_lam")
    DeleteWorkspace(slice_name)
    DeleteWorkspace(slice_name + "_ref")
    DeleteWorkspace("mon_slice")
    DeleteWorkspace("mon_rebin")
    DeleteWorkspace(DB + "_LAM")


def quickRef(run_numbers=[], trans_workspace_names=[], angles=[]):
    """Perform reflectometry reduction on each input run, and stitch the
    reduced workspaces together"""
    reduced_runs = ""
    for run_index in range(len(run_numbers)):
        # Set up the reduction properties
        run_name = str(run_numbers[run_index])
        trans_name = str(trans_workspace_names[run_index])
        properties = {
            "InputRunList": run_name + ".raw",
            "FirstTransmissionRunList": trans_name,
            "OutputWorkspaceBinned": run_name + "_IvsQ_binned",
            "OutputWorkspace": run_name + "_IvsQ",
            "OutputWorkspaceWavelength": run_name + "_IvsLam",
            "OutputWorkspaceTransmission": trans_name + "_LAM",
            "Debug": True,
        }
        # Set ThetaIn if the angles are given
        if angles:
            theta = angles[run_index]
            properties["ThetaIn"] = theta
            # Special case to set WavelengthMin for a specific angle
            if theta == 0.8:
                properties["WavelengthMin"] = 2.6
        # Do the reduction
        ReflectometryISISLoadAndProcess(**properties)
        DeleteWorkspace(trans_name + "_LAM")
        reduced_runs = reduced_runs + run_name + "_IvsQ_binned"
        if run_index < len(run_numbers) - 1:
            reduced_runs = reduced_runs + ","
    # Stitch the results
    first_run_name = str(run_numbers[0])
    dqq = NRCalculateSlitResolution(Workspace=first_run_name + "_IvsQ")
    stitched_name = stitchedWorkspaceName(run_numbers[0], run_numbers[-1])
    Stitch1DMany(InputWorkspaces=reduced_runs, OutputWorkspace=stitched_name, Params="-" + str(dqq), IndexOfReference=0)


def twoAngleFit(workspace_name, scalefactor, expected_fit_params, expected_fit_covariance):
    """Perform a fit on the given workspace and compare to the given results"""
    # Scale and fit
    Scale(InputWorkspace=workspace_name, OutputWorkspace=workspace_name + "_scaled", Factor=(1.0 / scalefactor))
    function_name = (
        "name=ReflectivityMulf, nlayer=1, Theta=2.3, ScaleFactor=1, AirSLD=0, BulkSLD=0, Roughness=0, BackGround=6.8e-06,"
        "Resolution=5.0, SLD_Layer0=1.0e-6, d_Layer0=20.0, Rough_Layer0=0.0, constraints=(0<SLD_Layer0, 0<d_Layer0),"
        "ties=(Theta=2.3, AirSLD=0, BulkSLD=0, Resolution=5.0, ScaleFactor=1.0, Roughness=0, Rough_Layer0=0)"
    )
    Fit(
        Function=function_name,
        InputWorkspace=workspace_name + "_scaled",
        IgnoreInvalidData="1",
        Output=workspace_name + "_fit",
        OutputCompositeMembers="1",
        ConvolveMembers="1",
    )
    # Get output tables
    params_table = mtd[workspace_name + "_fit_Parameters"]
    covariance_table = mtd[workspace_name + "_fit_NormalisedCovarianceMatrix"]
    # Print output info
    sld = round(params_table.cell(7, 1), 9)
    thick = round(params_table.cell(8, 1), 2)
    dNb = sld * thick
    print("dNb ", dNb)
    print("SLD ", sld)
    print("Thick ", thick)
    print("-----------")
    # Annoyingly, fitting/gsl seems unstable across different platforms so the results don't match
    # accurately. To get around this remove the offending workspaces from the reference and check
    # manually here instead with a more generous tolerance. This isn't ideal but should be enough.
    tolerance = 1e-2
    compareFitResults(params_table.toDict(), expected_fit_params, tolerance)
    compareFitResults(covariance_table.toDict(), expected_fit_covariance, tolerance)
    removeWorkspaces([workspace_name + "_fit_Parameters", workspace_name + "_fit_NormalisedCovarianceMatrix"])


def compareFitResults(results_dict, reference_dict, tolerance):
    """Compare the fit results to the reference. The output table workspaces from the fit
    should be converted to dicts before calling this function"""
    for key in reference_dict:
        if key == "Name":
            continue

        if key not in results_dict:
            raise ValueError("The column {0} was not found in the fit output table".format(key))

        reference_values = reference_dict[key]
        values_fitted = results_dict[key]
        if len(values_fitted) != len(reference_values):
            raise ValueError(
                "The values fitted and the reference values must be provided as lists (with the same "
                "number of elements).\nGot actual (length {0}) vs reference (length {1}):\n{2}\n{3}".format(
                    len(values_fitted), len(reference_values), values_fitted, reference_values
                )
            )

        for index, (value, expected) in enumerate(zip(values_fitted, reference_values)):
            if abs(value - expected) > tolerance:
                logger.error(
                    "For the parameter with index {0}, the value found '{1}' differs from "
                    "reference '{2}' by more than required tolerance '{3}'".format(index, value, expected, tolerance)
                )
                logger.error("These were the values found:         {0}".format(values_fitted))
                raise RuntimeError("Some results were not as accurate as expected. Please check the log " "messages for details")


def sliceAndReduceRun(run_number):
    """Generate 60 second time slices of the given run, and perform reflectometry
    reduction on each slice"""
    for slice_index in range(5):
        start = slice_index * 60
        stop = (slice_index + 1) * 60
        eventRef(run_number, 0.5, start, stop, DB="TRANS")


def testEventDataTimeSlicing(event_run_numbers):
    for run_number in event_run_numbers:
        sliceAndReduceRun(run_number)


def testReductionOfThreeAngleFringedSolidLiquidExample(run_numbers):
    quickRef(run_numbers, ["TRANS", "TRANS", "TRANS"])


def testReductionOfTwoAngleAirLiquidExample(run_numbers):
    quickRef(run_numbers, ["TRANS_SM", "TRANS_noSM"], angles=[0.8, 2.3])


def testFittingOfReducedData(run1_number, run2_number, expected_fit_params, expected_fit_covariance):
    # D2O run:
    CloneWorkspace(InputWorkspace="44984_85", OutputWorkspace="D2O_IvsQ_binned")
    # fit d2o to get scalefactor
    function_name = (
        "name=ReflectivityMulf, nlayer=0, Theta=2.3, ScaleFactor=1.0, AirSLD=0, BulkSLD=6.35e-6, Roughness=2.5,"
        "BackGround=3.0776e-06, Resolution=5.0, ties=(Theta=2.3, AirSLD=0, BulkSLD=6.35e-6, Resolution=5.0, Roughness=2.5)"
    )
    Fit(
        Function=function_name,
        InputWorkspace="D2O_IvsQ_binned",
        IgnoreInvalidData="1",
        Minimizer="Simplex",
        Output="D2O_fit",
        OutputCompositeMembers="1",
        ConvolveMembers="1",
        StartX="0.0015",
        EndX="0.3359",
    )
    scalefactor = round(mtd["D2O_fit_Parameters"].cell(1, 1), 3)
    # Create reduced workspace for test:
    quickRef([run1_number, run2_number], ["TRANS_SM", "TRANS_noSM"], angles=[0.8, 2.3])
    # Test fitting of the result:
    print("run ", str(run1_number))
    stitched_name = stitchedWorkspaceName(run1_number, run2_number)
    twoAngleFit(stitched_name, scalefactor, expected_fit_params, expected_fit_covariance)
