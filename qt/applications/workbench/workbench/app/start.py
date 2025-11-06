# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import argparse
import subprocess
import sys

from mantid.kernel.environment import is_linux
from mantid.kernel import Logger

if is_linux():
    import resource

from mantidqt.utils.qt import plugins

# Find Qt plugins for development builds on some platforms
plugins.setup_library_paths()

# Importing resources loads the data in. This must be imported before the
# QApplication is created or paths to Qt's resources will not be set up correctly
from workbench.config import APPNAME, ORG_DOMAIN, ORGANIZATION  # noqa: E402

import workbench.app.workbench_process as wp  # noqa: E402


def start_error_reporter(workbench_pid):
    """
    Used to start the error reporter if the program has segfaulted.
    """
    from mantidqt.dialogs.errorreports import main as errorreports_main

    errorreports_main.main(
        ["--application", APPNAME, "--workbench_pid", workbench_pid, "--orgname", ORGANIZATION, "--orgdomain", ORG_DOMAIN]
    )


def _setup_core_dump_files():
    """
    This is done so that the error reporter can locate and read any core files produced.
    This allows us to recover traces from c++ based crashes.
    """
    try:
        resource.setrlimit(resource.RLIMIT_CORE, (resource.RLIM_INFINITY, resource.getrlimit(resource.RLIMIT_CORE)[1]))
    except ValueError as e:
        log = Logger("Mantid Start")
        log.warning(f"Problem when enabling core dumps\n{str(e)}")


def _raise_open_file_descriptor_limit():
    """
    Done to avoid the limit being reached when running pystack in the error reporter
    """
    try:
        nofile_max = resource.getrlimit(resource.RLIMIT_NOFILE)[1]
        resource.setrlimit(resource.RLIMIT_NOFILE, (nofile_max, nofile_max))
    except ValueError as e:
        log = Logger("Mantid Start")
        log.warning(f"Problem when raising limit for number of open file descriptors\n{str(e)}")


def start(options: argparse.ArgumentParser):
    """Start workbench based on the given options

    :param options: An object describing the command-line arguments passed in
    """
    if options.single_process:
        wp.initialise_qapp_and_launch_workbench(options)
    else:
        # Mantid's FrameworkManagerImpl::Instance Python export uses a process-wide static flag to ensure code
        # only runs once (see Instance function in Framework/PythonInterface/mantid/api/src/Exports/FrameworkManager.cpp).
        # The default start method on Unix ('fork') inherits resources such as these flags from the parent.
        # We require the start method to be 'spawn' so that we do not inherit these resources from the parent process,
        # this is already the default on Windows/macOS.
        # This will mean the relevant 'atexit' code will execute in the child process, and therefore the
        # FrameworkManager and UsageService will be shutdown as expected.
        launch_command = [str(sys.executable), str(wp.__file__)]
        if options.script:
            launch_command.append(str(options.script))
        if options.execute:
            launch_command.append("--execute")
        if options.quit:
            launch_command.append("--quit")

        if is_linux():
            # currently, reading coredump files for error reports is only supported on linux
            _raise_open_file_descriptor_limit()
            workbench_process = subprocess.Popen(launch_command, preexec_fn=_setup_core_dump_files)
        else:
            workbench_process = subprocess.Popen(launch_command)

        workbench_pid = str(workbench_process.pid)
        workbench_process.wait()

        # handle exit information
        exit_code = workbench_process.returncode if workbench_process.returncode is not None else 1
        if exit_code != 0:
            # start error reporter if requested
            if not options.no_error_reporter:
                start_error_reporter(workbench_pid)

            # a signal was emited so raise the signal from the application
            if exit_code < 0:
                import signal

                try:
                    sig_code = signal.Signals(abs(exit_code))
                    name = sig_code.name
                    print("Received signal:", name)
                except ValueError:
                    pass
                signal.raise_signal(sig_code)  # this will exit the mainloop

            # application returned non-zero that wasn't interpreted as a signal, return it as an exit code
            sys.exit(exit_code)
