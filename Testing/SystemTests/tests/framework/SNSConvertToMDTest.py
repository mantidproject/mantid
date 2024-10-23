# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init
import systemtesting
from mantid.api import mtd, AlgorithmManager
from mantid.kernel import config
from mantid.simpleapi import ConvertToMD, DeleteWorkspace, DgsReduction, Load, LoadMD, SaveMD, SetGoniometer, SetUB

######################################################################
# Common configuration
# Main data file /SNS/SEQ/IPTS-4783/data
DATA_FILE = "SEQ_11499_event.nxs"
# Vanadium file
VAN_FILE = "SEQ_van.nxs"
# Initial energy guess
E_GUESS = 50
# Energy bins: Emin, Estep, Emax
E_RANGE = "-10.0,0.2,45.0"
#######################################################################


def makeOutputName(ws_name, dohist, doproj):
    md_ws_name = ws_name + "_md"
    tag = ""
    if dohist:
        tag += "h"
    else:
        tag += "e"
    if doproj:
        tag += "wp"
    else:
        tag += "np"

    md_ws_name += "_" + tag
    return md_ws_name


def execReduction(dohist, doproj):
    # Set the facility
    config["default.facility"] = "SNS"
    # SPE workspace name
    workspace_name = "reduced"
    # Run the reduction
    DgsReduction(
        SampleInputFile=DATA_FILE,
        IncidentBeamNormalisation="ByCurrent",
        OutputWorkspace=workspace_name,
        IncidentEnergyGuess=E_GUESS,
        EnergyTransferRange=E_RANGE,
        SofPhiEIsDistribution=dohist,
        DetectorVanadiumInputFile=VAN_FILE,
        UseProcessedDetVan=True,
    )

    # Set the goniometer. Add a rotation angle fix as well.
    SetGoniometer(Workspace=workspace_name, Axis0="CCR13VRot,0,1,0,1", Axis1="49.73,0,1,0,1")

    # Set the information for the UB matrix
    SetUB(Workspace=workspace_name, a=3.643, b=3.643, c=5.781, alpha=90, beta=90, gamma=120, u="1,1,0", v="0,0,1")

    # Create the MDEventWorkspace
    md_output_ws = makeOutputName(workspace_name, dohist, doproj)

    if not doproj:
        ConvertToMD(
            InputWorkspace=workspace_name,
            OutputWorkspace=md_output_ws,
            QDimensions="Q3D",
            MinValues="-5,-5,-5,-10",
            QConversionScales="HKL",
            MaxValues="5,5,5,45",
            MaxRecursionDepth="1",
        )
    else:
        ConvertToMD(
            InputWorkspace=workspace_name,
            OutputWorkspace=md_output_ws,
            QDimensions="Q3D",
            MinValues="-5,-5,-5,-10",
            QConversionScales="HKL",
            MaxValues="5,5,5,45",
            MaxRecursionDepth="1",
            Uproj="1,1,0",
            Vproj="1,-1,0",
            Wproj="0,0,1",
        )

    # Remove SPE workspace
    DeleteWorkspace(Workspace=workspace_name)

    return md_output_ws


def validateMD(result, reference, tol=1.0e-5, class_name="dummy", mismatchName=None):
    """Returns the name of the workspace & file to compare"""
    # elf.disableChecking.append('SpectraMap')
    # elf.disableChecking.append('Instrument')

    valNames = [result, reference]

    if reference not in mtd:
        Load(Filename=reference, OutputWorkspace=valNames[1])

    checker = AlgorithmManager.create("CompareMDWorkspaces")
    checker.setLogging(True)
    checker.setPropertyValue("Workspace1", result)
    checker.setPropertyValue("Workspace2", valNames[1])
    checker.setPropertyValue("Tolerance", str(tol))
    checker.setPropertyValue("IgnoreBoxID", "1")
    checker.setPropertyValue("CheckEvents", "1")

    checker.execute()
    if checker.getPropertyValue("Equals") != "1":
        print(" Workspaces do not match, result: ", checker.getPropertyValue("Result"))
        print(" Test {0} fails".format(class_name))
        if mismatchName:
            targetFilename = class_name + mismatchName + "-mismatch.nxs"
        else:
            targetFilename = class_name + "-mismatch.nxs"

        SaveMD(InputWorkspace=valNames[0], Filename=targetFilename)
        return False
    else:
        return True


class SNSConvertToMDNoHistNoProjTest(systemtesting.MantidSystemTest):
    truth_file = "SEQ_11499_md_enp.nxs"
    output_ws = None
    tolerance = 0.0
    gold_ws_name = ""

    def requiredMemoryMB(self):
        """Require about 2.5GB free"""
        return 2500

    def requiredFiles(self):
        files = [self.truth_file, DATA_FILE]
        return files

    def runTest(self):
        self.output_ws = execReduction(False, False)

        self.gold_ws_name = self.truth_file.split(".")[0] + "_golden"
        LoadMD(self.truth_file, OutputWorkspace=self.gold_ws_name)

    def validate(self):
        self.tolerance = 1.0e-1
        return validateMD(self.output_ws, self.gold_ws_name, self.tolerance, self.__class__.__name__)


class SNSConvertToMDHistNoProjTest(systemtesting.MantidSystemTest):
    truth_file = "SEQ_11499_md_hnp.nxs"
    output_ws = None
    tolerance = 0.0
    gold_ws_name = ""

    def requiredMemoryMB(self):
        """Require about 2.5GB free"""
        return 2500

    def requiredFiles(self):
        config.appendDataSearchDir("/home/builder/data/SystemTests/AnalysisTests/ReferenceResults/")
        files = [self.truth_file, DATA_FILE]
        return files

    def runTest(self):
        self.output_ws = execReduction(True, False)

        self.gold_ws_name = self.truth_file.split(".")[0] + "_golden"
        LoadMD(self.truth_file, OutputWorkspace=self.gold_ws_name)

    def validate(self):
        self.tolerance = 1.0e-1
        return validateMD(self.output_ws, self.gold_ws_name, self.tolerance, self.__class__.__name__, self.gold_ws_name)


class SNSConvertToMDNoHistProjTest(systemtesting.MantidSystemTest):
    truth_file = "SEQ_11499_md_ewp.nxs"
    output_ws = None
    tolerance = 0.0
    gold_ws_name = ""

    def requiredMemoryMB(self):
        """Require about 2.5GB free"""
        return 2500

    def requiredFiles(self):
        files = [self.truth_file, DATA_FILE]
        return files

    def runTest(self):
        self.output_ws = execReduction(False, True)

        self.gold_ws_name = self.truth_file.split(".")[0] + "_golden"
        LoadMD(self.truth_file, OutputWorkspace=self.gold_ws_name)

    def validate(self):
        self.tolerance = 1.0e-3
        return validateMD(self.output_ws, self.gold_ws_name, self.tolerance, self.__class__.__name__, self.gold_ws_name)
        # return (self.output_ws, self.gold_ws_name)


class SNSConvertToMDHistProjTest(systemtesting.MantidSystemTest):
    truth_file = "SEQ_11499_md_hwp.nxs"
    output_ws = None
    tolerance = 0.0
    gold_ws_name = ""

    def requiredMemoryMB(self):
        """Require about 2.5GB free"""
        return 2500

    def requiredFiles(self):
        files = [self.truth_file, DATA_FILE]
        return files

    def runTest(self):
        self.output_ws = execReduction(True, True)

        self.gold_ws_name = self.truth_file.split(".")[0] + "_golden"
        LoadMD(self.truth_file, OutputWorkspace=self.gold_ws_name)

    def validate(self):
        self.tolerance = 1.0e-3
        return validateMD(self.output_ws, self.gold_ws_name, self.tolerance, self.__class__.__name__, self.gold_ws_name)
        # return (self.output_ws, self.gold_ws_name)
