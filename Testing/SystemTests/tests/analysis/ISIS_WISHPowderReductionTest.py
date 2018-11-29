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


class WISHPowderReductionTest(MantidSystemTest):
    # still missing required files check with ./systemtest -R PowderReduction --showskipped
    def requiredFiles(self):
        input_files = ["vana19612-1foc-SF-SS.nxs", "vana19612-2foc-SF-SS.nxs", "vana19612-3foc-SF-SS.nxs",
                       "vana19612-4foc-SF-SS.nxs", "vana19612-5foc-SF-SS.nxs", "vana19612-6foc-SF-SS.nxs",
                       "vana19612-7foc-SF-SS.nxs", "vana19612-8foc-SF-SS.nxs", "vana19612-9foc-SF-SS.nxs",
                       "vana19612-10foc-SF-SS.nxs", "emptyinst19618-1foc-SF-S.nxs", "emptyinst19618-2foc-SF-S.nxs",
                       "emptyinst19618-3foc-SF-S.nxs", "emptyinst19618-4foc-SF-S.nxs", "emptyinst19618-5foc-SF-S.nxs",
                       "emptyinst19618-6foc-SF-S.nxs", "emptyinst19618-7foc-SF-S.nxs", "emptyinst19618-8foc-SF-S.nxs",
                       "emptyinst19618-9foc-SF-S.nxs", "emptyinst19618-10foc-SF-S.nxs"]

        input_files = [os.path.join(calibration_dir, files) for files in input_files]
        return input_files

    def cleanup(self):
        shutil.rmtree(output_dir)

    def runTest(self):
        os.makedirs(output_dir)
        wish_test = Wish("__main__", calibration_dir+"/", output_dir + "/", True, input_dir)
        wish_test.reduce()
        self.clearWorkspaces()

    def validate(self):
        #return "w40503-1_10foc", "WISH40503-1_10raw.nxs", \
        #       "w40503-2_9foc",  "WISH40503-2_9raw.nxs", \
        #       "w40503-3_8foc",  "WISH40503-3_8raw.nxs", \
        #       "w40503-4_7foc",  "WISH40503-4_7raw.nxs", \
        return "w40503-5_6foc",  "WISH40503-5_6raw.nxs"

    def clearWorkspaces(self):
        deletews = ["w40503-" + str(i) + "foc" for i in range(5, 7)]
        for ws in deletews:
            mantid.DeleteWorkspace(ws)
            mantid.DeleteWorkspace(ws + "-d")

    # Skip test when on builds as extremely slow, run only as reversion test for wish script
    def skipTests(self):
        return False
