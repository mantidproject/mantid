#!/usr/bin/env python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import inspect
import os
import sys
import time
import importlib.util

# Prevents errors in systemtests that use matplotlib directly
os.environ["MPLBACKEND"] = "Agg"

#########################################################################
# Set up the command line options
#########################################################################

start_time = time.time()

DEFAULT_QT_API = "pyqt5"
THIS_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))
DEFAULT_FRAMEWORK_LOC = os.path.realpath(os.path.join(THIS_MODULE_DIR, "..", "lib", "systemtests"))
SANS_TEST_LOC = os.path.realpath(os.path.join(THIS_MODULE_DIR, "..", "tests", "framework", "ISIS", "SANS"))
DATA_DIRS_LIST_PATH = os.path.join(THIS_MODULE_DIR, "datasearch-directories.txt")
SAVE_DIR_LIST_PATH = os.path.join(THIS_MODULE_DIR, "defaultsave-directory.txt")
DEFAULT_LOG_LEVEL = "information"
DEFAULT_ARCHIVE_SEARCH = True
DEFAULT_MAKE_PROP = True
DEFAULT_EXECUTABLE = sys.executable


def kill_children(processes):
    for process in processes:
        process.terminate()
    # Exit with status 1 - NOT OK
    sys.exit(1)


def main(argv):
    # Set the Qt version to use during the system tests
    os.environ["QT_API"] = DEFAULT_QT_API

    # import the system testing framework
    sys.path.append(DEFAULT_FRAMEWORK_LOC)
    import systemtesting

    # allow PythonInterface/test to be discoverable
    sys.path.append(systemtesting.FRAMEWORK_PYTHONINTERFACE_TEST_DIR)

    sys.path.append(SANS_TEST_LOC)
    #########################################################################
    # Configure mantid
    #########################################################################

    # Parse files containing the search and save directories
    with open(DATA_DIRS_LIST_PATH, "r") as f_handle:
        data_paths = f_handle.read().strip()

    with open(SAVE_DIR_LIST_PATH, "r") as f_handle:
        save_dir = f_handle.read().strip()

    # Configure properties file
    mtdconf = systemtesting.MantidFrameworkConfig(
        loglevel=DEFAULT_LOG_LEVEL, data_dirs=data_paths, save_dir=save_dir, archivesearch=DEFAULT_ARCHIVE_SEARCH
    )
    if DEFAULT_MAKE_PROP:
        mtdconf.config()

    #########################################################################
    # Generate list of tests
    #########################################################################

    path_to_test = argv[1]
    test_dir_name = os.path.dirname(path_to_test)
    test_file_name = os.path.basename(path_to_test)
    test_module_name = os.path.splitext(test_file_name)[0]
    test_spec = importlib.util.spec_from_file_location(test_module_name, path_to_test)
    test_module = importlib.util.module_from_spec(test_spec)
    test_spec.loader.exec_module(test_module)
    test_classes = dict(inspect.getmembers(test_module, inspect.isclass))

    test_class_names = [test_class for test_class in test_classes if
                    systemtesting.isValidTestClass(test_classes[test_class]) and test_class != "MantidSystemTest"]

    if not test_class_names:
        raise NameError(f"No test classes found in system test module: {test_module_name}.")

    runner = systemtesting.TestRunner(
        executable=DEFAULT_EXECUTABLE,
        # see InstallerTests.py for why lstrip is required
        exec_args="--classic",
        escape_quotes=True,
    )

    results = dict()
    for test_class_name in test_class_names:
        script_obj = systemtesting.TestScript(test_dir_name, test_module_name, test_class_name, False)
        results[test_class_name] = runner.start_in_current_process(script_obj)

    #########################################################################
    # Process Results
    #########################################################################

    failure = False
    exit_codes = list(results.values())
    if any(exit_code[0] is not systemtesting.TestRunner.SUCCESS_CODE for exit_code in exit_codes):
        failure = True

    #########################################################################
    # Cleanup
    #########################################################################

    # Put the configuration back to its original state
    mtdconf.restoreconfig()

    if failure:
        # The lack of details here is due to ctest handling the stdout dump on failures.
        raise RuntimeError()


if __name__ == "__main__":
    main(sys.argv)
