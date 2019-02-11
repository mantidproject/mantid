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
# just test 5 and 6 to save time as process is the same for all other pairs
panels = [5, 6]
linked_panels = {
    1: 10,
    2: 9,
    3: 8,
    4: 7,
    5: 6
}


class WISHPowderReductionNoAbsorptionTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["vana19612-{}foc-SF-SS.nxs".format(panel) for panel in panels]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        create_folder()
        wish_test = Wish(calibration_dir, output_dir, True, input_dir + "/", False)
        runs = [40503]

        wish_test.reduce(runs, panels)
        self.clearWorkspaces()

    def validate(self):
        self.tolerence = 1.e-8
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

    def requiredMemoryMB(self):
        return 12000


class WISHPowderReductionTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["vana19612-{}foc-SF-SS.nxs".format(panel) for panel in panels]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        create_folder()
        wish_test = Wish(calibration_dir, output_dir, True, input_dir + "/")
        runs = [40503]

        wish_test.reduce(runs, panels)
        self.clearWorkspaces()

    def validate(self):
        self.tolerence = 1.e-8
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

    def requiredMemoryMB(self):
        return 12000


class WISHPowderReductionCreateVanadiumTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["emptyinst19618-{}foc-SF-S.nxs".format(panel) for panel in panels]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        if os.path.isdir(output_dir):
            shutil.rmtree(output_dir)

    def runTest(self):
        create_folder()
        wish_test = Wish(calibration_dir, output_dir, True, input_dir + "/")
        wish_test.create_vanadium_run(19612, 19618, panels)

    def validate(self):
        self.tolerence = 1.e-8
        validation_files = []
        for panel in [x for x in panels if x < 6]:
            validation_files = validation_files + ["w19612-{}foc".format(panel),
                                                   "vana19612-{}foc-SF-SS.nxs".format(panel)]
        return validation_files

    def requiredMemoryMB(self):
        return 12000


def create_folder():
    # make folder in try catch because we can't guarantee that the cleanup has run, once we dont need to support
    # python 2 we can use tempfile.TemporaryDirectory() which is automatically deleted like tempfile is
    try:
        os.makedirs(output_dir)
    except OSError:
        return
