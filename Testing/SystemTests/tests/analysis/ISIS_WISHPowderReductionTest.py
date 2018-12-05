from systemtesting import MantidSystemTest
from wish.reduce import Wish

from mantid import config
import mantid.simpleapi as mantid
import os
import shutil

DIRS = config['datasearch.directories'].split(';')

# Relative to system data folder
working_folder_name = "WISH"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = "Cal"

# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_dir = os.path.join(input_dir, calibration_folder_name)
panels = [5, 6]
linked_panels = {
    1: 10,
    2: 9,
    3: 8,
    4: 7,
    5: 6
}


class WISHPowderReductionTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["vana19612-1foc-SF-SS.nxs", "vana19612-2foc-SF-SS.nxs", "vana19612-3foc-SF-SS.nxs",
                       "vana19612-4foc-SF-SS.nxs", "vana19612-5foc-SF-SS.nxs", "vana19612-6foc-SF-SS.nxs",
                       "vana19612-7foc-SF-SS.nxs", "vana19612-8foc-SF-SS.nxs", "vana19612-9foc-SF-SS.nxs",
                       "vana19612-10foc-SF-SS.nxs"]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        os.makedirs(output_dir)
        wish_test = Wish(calibration_dir + "/", output_dir + "/", True, input_dir + "/", False)
        runs = [40503]

        wish_test.reduce(runs, panels)
        self.clearWorkspaces()

    def validate(self):
        validation_files = []
        for panel in [x for x in panels if x < 6]:
            validation_files = validation_files + \
                               ["w40503-{0}_{1}foc".format(panel, linked_panels.get(panel)),
                                "WISH40503-{0}_{1}no_absorb_raw.nxs".format(panel, linked_panels.get(panel))]
        return validation_files

    def clearWorkspaces(self):
        deletews = ["w40503-{}foc".format(panel) for panel in panels]
        for ws in deletews:
            mantid.DeleteWorkspace(ws)
            mantid.DeleteWorkspace(ws + "-d")

    # Skip test when on builds as extremely slow, run only as reversion test for wish script
    def skipTests(self):
        return False


class WISHPowderReductionNoAbsorptionTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["vana19612-1foc-SF-SS.nxs", "vana19612-2foc-SF-SS.nxs", "vana19612-3foc-SF-SS.nxs",
                       "vana19612-4foc-SF-SS.nxs", "vana19612-5foc-SF-SS.nxs", "vana19612-6foc-SF-SS.nxs",
                       "vana19612-7foc-SF-SS.nxs", "vana19612-8foc-SF-SS.nxs", "vana19612-9foc-SF-SS.nxs",
                       "vana19612-10foc-SF-SS.nxs"]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        os.makedirs(output_dir)
        wish_test = Wish(calibration_dir + "/", output_dir + "/", True, input_dir + "/")
        runs = [40503]

        wish_test.reduce(runs, panels)
        self.clearWorkspaces()

    def validate(self):
        validation_files = []
        for panel in [x for x in panels if x < 6]:
            validation_files = validation_files + ["w40503-{0}_{1}foc".format(panel, linked_panels.get(panel)),
                                                   "WISH40503-{0}_{1}raw.nxs".format(panel, linked_panels.get(panel))]
        return validation_files

    def clearWorkspaces(self):
        deletews = ["w40503-{}foc".format(panel) for panel in panels]
        for ws in deletews:
            mantid.DeleteWorkspace(ws)
            mantid.DeleteWorkspace(ws + "-d")

    # Skip test when on builds as extremely slow, run only as reversion test for wish script
    def skipTests(self):
        return False


class WISHPowderReductionCreateVanadiumTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["emptyinst19618-2foc-SF-S.nxs",
                       "emptyinst19618-3foc-SF-S.nxs", "emptyinst19618-4foc-SF-S.nxs", "emptyinst19618-5foc-SF-S.nxs",
                       "emptyinst19618-6foc-SF-S.nxs", "emptyinst19618-7foc-SF-S.nxs", "emptyinst19618-8foc-SF-S.nxs",
                       "emptyinst19618-9foc-SF-S.nxs", "emptyinst19618-10foc-SF-S.nxs"]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        os.makedirs(output_dir)
        wish_test = Wish(calibration_dir + "/", output_dir + "/", True, input_dir)
        wish_test.create_vanadium_run(19612, 19618, panels)

    def validate(self):
        validation_files = []
        for panel in [x for x in panels if x < 6]:
            validation_files = validation_files + ["w19612-{}foc".format(panel),
                                                   "vana19612-{}foc-SF-SS.nxs".format(panel)]
        print(validation_files)
        return validation_files

    # Skip test when on builds as extremely slow, run only as reversion test for wish script
    def skipTests(self):
        return False
