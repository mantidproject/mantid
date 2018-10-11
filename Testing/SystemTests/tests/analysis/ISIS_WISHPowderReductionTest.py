from stresstesting import MantidStressTest
import wish.reduce

from mantid import config
import os


DIRS = config['datasearch.directories'].split(';')

# Relative to system data folder
working_folder_name = "WISH"

# Relative to working folder
input_folder_name = "input"
output_folder_name = "output"

# Relative to input folder
calibration_folder_name = "calibration"



# Generate paths for the tests
# This implies DIRS[0] is the system test data folder
working_dir = os.path.join(DIRS[0], working_folder_name)

input_dir = os.path.join(working_dir, input_folder_name)
output_dir = os.path.join(working_dir, output_folder_name)

calibration_dir = os.path.join(input_dir, calibration_folder_name)

class WISHPowderReductionTest(MantidStressTest):

    def requiredFiles(self):
        return ["WISHvana41865-1foc.nxs","WISHvana41865-2foc.nxs","WISHvana41865-3foc.nxs","WISHvana41865-4foc.nxs",
                "WISHvana41865-5foc.nxs","WISHvana41865-6foc.nxs","WISHvana41865-7foc.nxs","WISHvana41865-8foc.nxs",
                "WISHvana41865-9foc.nxs","WISHvana41865-10foc.nxs","emptyinst38581-1foc.nxs","emptyinst38581-2foc.nxs",
                "emptyinst38581-3foc.nxs","emptyinst38581-4foc.nxs","emptyinst38581-5foc.nxs","emptyinst38581-6foc.nxs",
                "emptyinst38581-7foc.nxs","emptyinst38581-8foc.nxs","emptyinst38581-9foc.nxs",
                "emptyinst38581-10foc.nxs"]
    #def requiredMemoryMB(self):
        # Need lots of memory for full WISH dataset
     #   return 20000

    def cleanup(self):
        pass

    def runTest(self):

        wish.reduce.Wish_Run("__main__", calibration_dir, input, output_dir)

    def validate(self):
        return "w41870-2_9foc-d", "WISH41870-2_9raw.nxs", \
               "w41870-3_8foc-d", "WISH41870-3_8raw.nxs", \
               "w41870-4_7foc-d", "WISH41870-4_7raw.nxs", \
               "w41870-5_6foc-d", "WISH41870-5_6raw.nxs"

