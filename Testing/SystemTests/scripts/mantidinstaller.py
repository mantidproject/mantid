# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines classes for handling installation
"""
import platform
import os
import glob
import sys
import subprocess
import time

# global script path
scriptLog = None


def createScriptLog(path):
    global scriptLog
    scriptLog = open(path, "w")


def stop(installer):
    """Save the log, uninstall the package and exit with error code 0"""
    try:
        installer.uninstall()
    except Exception as exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))
    scriptLog.close()
    sys.exit(0)


def log(txt):
    """Write text to the script log file"""
    if scriptLog is None:
        return
    if txt and len(txt) > 0:
        scriptLog.write(txt)
        if not txt.endswith("\n"):
            scriptLog.write("\n")
        print(txt)


def failure(installer):
    """Report failure of test(s), try to uninstall package and exit with code 1"""
    try:
        installer.uninstall()
    except Exception as exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))

    log("Tests failed")
    print("Tests failed")
    sys.exit(1)


def scriptfailure(txt, installer=None):
    """Report failure of this script, try to uninstall package and exit with code 1"""
    if txt:
        log(txt)
    if installer is not None:
        try:
            installer.uninstall()
        except Exception:
            log("Could not uninstall package %s " % str(installer))
    scriptLog.close()
    sys.exit(1)


def get_installer(package_dir, do_install=True):
    """
    Creates the correct class for the current platform
        @param package_dir :: The directory to search for packages
        @param do_install :: True if installation is to be performed
    """
    import os

    if os.environ.get("MANTID_FRAMEWORK_CONDA_SYSTEMTEST"):
        return CondaInstaller(package_dir, do_install)
    else:
        raise scriptfailure("Unsupported platform")


def run(cmd):
    """Run a command in a subprocess"""
    try:
        stdout = subprocess.check_output(cmd, shell=True, stderr=subprocess.STDOUT).decode("utf-8")
    except subprocess.CalledProcessError as exc:
        log(f"Error in subprocess {exc}")
        raise
    log(stdout)
    return stdout


class MantidInstaller(object):
    """
    Base-class for installer objects
    """

    mantidInstaller = None
    no_uninstall = False
    python_cmd = None
    python_args = "--classic"

    def __init__(self, package_dir, filepattern, do_install):
        """Initialized with a pattern to
        find a path to an installer
        """
        # Glob for packages
        matches = glob.glob(os.path.join(package_dir, filepattern))
        if len(matches) > 0:
            # This will put the release mantid packages at the start and the nightly ones at the end
            # with increasing version numbers
            matches.sort()
        # Take the last one as it will have the highest version number
        if len(matches) > 0:
            self.mantidInstaller = os.path.join(os.getcwd(), matches[-1])
            log("Found package " + self.mantidInstaller)
        else:
            raise RuntimeError('Unable to find installer package in "%s"' % os.getcwd())
        self.no_uninstall = not do_install
        if not do_install:
            log("No install requested. Assuming this package has been installed externally.")

    def install(self):
        self.do_install()

    def do_install(self):
        raise NotImplementedError("Override the do_install method")

    def uninstall(self):
        if not self.no_uninstall:
            self.do_uninstall()

    def do_uninstall(self):
        raise NotImplementedError("Override the do_uninstall method")


class CondaInstaller(MantidInstaller):

    python_args = ""  # just normal python

    def __init__(self, package_dir, do_install=True):
        filepattern = "mantid-framework*.tar.bz2"
        MantidInstaller.__init__(self, package_dir, filepattern, do_install)
        package = os.path.basename(self.mantidInstaller)
        bindir = os.path.dirname(sys.executable)
        prefix = os.path.dirname(bindir)
        self.conda_mantid_env_prefix = prefix
        self.python_cmd = sys.executable

    def do_install(self):
        """Uses gdebi to run the install"""
        thisdir = os.path.dirname(__file__)
        script = os.path.join(thisdir, "install_conda_mantid.sh")
        run("%s %s" % (script, self.mantidInstaller))

    def do_uninstall(self):
        """Removes the conda package"""
        # run('rm -rf %s' % self.conda_mantid_env_prefix)


# -------------------------------------------------------------------------------
# Main
# -------------------------------------------------------------------------------
# If called as a standalone script then this can be used to install/uninstall
# Mantid
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Commands available: install, uninstall")
    parser.add_argument("command", choices=["install", "uninstall"], help="command to run")
    parser.add_argument("directory", help="package directory")

    options = parser.parse_args()

    package_dir = os.path.abspath(options.directory)
    print("Searching for packages in '%s'" % package_dir)
    installer = get_installer(package_dir)
    if options.command == "install":
        print("Installing package '%s'" % installer.mantidInstaller)
        installer.install()
    elif options.command == "uninstall":
        print("Removing package '%s'" % installer.mantidInstaller)
        installer.uninstall()
