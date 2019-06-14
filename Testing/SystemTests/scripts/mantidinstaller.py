# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
"""Defines classes for handling installation
"""
import platform
import os
import glob
import sys
import subprocess
import time

scriptLog = None


def createScriptLog(path):
    global scriptLog
    scriptLog = open(path,'w')


def stop(installer):
    ''' Save the log, uninstall the package and exit with error code 0 '''
    try:
        installer.uninstall()
    except Exception as exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))
    scriptLog.close()
    sys.exit(0)


def log(txt):
    ''' Write text to the script log file '''
    if scriptLog is None:
        return
    if txt and len(txt) > 0:
        scriptLog.write(txt)
        if not txt.endswith('\n'):
            scriptLog.write('\n')
        print(txt)


def failure(installer):
    ''' Report failure of test(s), try to uninstall package and exit with code 1 '''
    try:
        installer.uninstall()
    except Exception as exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))
        pass

    log('Tests failed')
    print('Tests failed')
    sys.exit(1)


def scriptfailure(txt, installer=None):
    '''Report failure of this script, try to uninstall package and exit with code 1 '''
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
    # == for testing conda build of mantid-framework ==========
    import os
    if os.environ.get('MANTID_FRAMEWORK_CONDA_SYSTEMTEST'):
        return CondaInstaller(package_dir, do_install)
    # =========================================================
    system = platform.system()
    if system == 'Windows':
        return NSISInstaller(package_dir, do_install)
    elif system == 'Linux':
        dist = platform.dist()
        if dist[0] == 'Ubuntu':
            return DebInstaller(package_dir, do_install)
        elif dist[0].lower() == 'redhat' or dist[0].lower() == 'fedora' or dist[0].lower() == 'centos':
            return RPMInstaller(package_dir, do_install)
        else:
            scriptfailure('Unknown Linux flavour: %s' % str(dist))
    elif system == 'Darwin':
        return DMGInstaller(package_dir, do_install)
    else:
        raise scriptfailure("Unsupported platform")


def run(cmd):
    """Run a command in a subprocess"""
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        stdout, stderr = p.communicate()
        if p.returncode != 0:
            raise Exception('Returned with code '+str(p.returncode)+'\n'+ stdout)
    except Exception as err:
        log('Error in subprocess %s:\n' % str(err))
        raise
    log(stdout)
    return stdout


class MantidInstaller(object):
    """
    Base-class for installer objects
    """
    mantidInstaller = None
    mantidPlotPath = None
    no_uninstall = False
    python_cmd = None
    python_args = "--classic"

    def __init__(self, package_dir, filepattern,
                 do_install):
        """Initialized with a pattern to
        find a path to an installer
        """
        # Glob for packages
        matches = glob.glob(os.path.join(package_dir, filepattern))
        if len(matches) > 0:
            # This will put the release mantid packages at the start and the nightly ones at the end
            # with increasing version numbers
            matches.sort()
            # Make sure we don't get Vates
            for match in matches:
                if 'vates'in match:
                    matches.remove(match)
        # Take the last one as it will have the highest version number
        if len(matches) > 0:
            self.mantidInstaller = os.path.join(os.getcwd(), matches[-1])
            log("Found package" + self.mantidInstaller)
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


class NSISInstaller(MantidInstaller):
    """Uses an NSIS installer
    to install Mantid
    """

    def __init__(self, package_dir, do_install):
        MantidInstaller.__init__(self, package_dir, 'mantid*.exe', do_install)
        package = os.path.basename(self.mantidInstaller)
        install_prefix = 'C:/'
        if 'mantidnightly' in package:
            install_prefix += 'MantidNightlyInstall'
        elif 'mantidunstable' in package:
            install_prefix += 'MantidUnstableInstall'
        else:
            install_prefix += 'MantidInstall'

        self.uninstallPath = install_prefix + '/Uninstall.exe'
        self.mantidPlotPath = install_prefix + '/bin/launch_mantidplot.bat'
        self.python_cmd = install_prefix + '/bin/mantidpython.bat'

    def do_install(self):
        """
            The NSIS installer spawns a new process and returns immediately.
            We use the start command with the /WAIT option to make it stay around
            until completion.
            The chained "&& exit 1" ensures that if the return code of the
            installer > 0 then the resulting start process exits with a return code
            of 1 so we can pick this up as a failure
        """
        run('start "Installer" /B /WAIT ' + self.mantidInstaller + ' /S')

    def do_uninstall(self):
        "Runs the uninstall exe"
        # The NSIS uninstaller actually runs a new process & detaches itself from the parent
        # process so that it is able to remove itself. This means that the /WAIT has no affect
        # because the parent appears to finish almost immediately
        run(self.uninstallPath + ' /S')
        # Wait for 30 seconds for it to finish
        log("Waiting 30 seconds for uninstaller to finish")
        time.sleep(30)


class LinuxInstaller(MantidInstaller):
    """Defines common properties for linux-based packages"""

    def __init__(self, package_dir, filepattern,
                 do_install):
        MantidInstaller.__init__(self, package_dir, filepattern, do_install)
        package = os.path.basename(self.mantidInstaller)
        install_prefix = '/opt'
        if 'mantidnightly' in package:
            install_prefix += '/mantidnightly'
        elif 'mantidunstable' in package:
            install_prefix += '/mantidunstable'
        else:
            install_prefix += '/Mantid'

        if 'python3' in package:
            install_prefix += '-python3'

        self.mantidPlotPath = install_prefix + '/bin/MantidPlot'
        self.python_cmd = install_prefix + '/bin/mantidpython'


class DebInstaller(LinuxInstaller):
    """Uses a deb package to install mantid
    """

    def __init__(self, package_dir, do_install):
        LinuxInstaller.__init__(self, package_dir, 'mantid*.deb', do_install)

    def do_install(self):
        """Uses gdebi to run the install
        """
        run('sudo gdebi -n ' + self.mantidInstaller)

    def do_uninstall(self):
        """Removes the debian package
        """
        package_name = os.path.basename(self.mantidInstaller).split("_")[0]
        run('sudo dpkg --purge %s' % package_name)


class RPMInstaller(LinuxInstaller):
    """Uses a rpm package to install mantid
    """

    def __init__(self, package_dir, do_install):
        LinuxInstaller.__init__(self, package_dir, 'mantid*.rpm', do_install)

    def do_install(self):
        """Uses yum to run the install. Current user must be in sudoers
        """
        try:
            run('sudo rpm -e ' + self.mantidInstaller)
        except Exception:
            # Assume it doesn't exist
            pass
        try:
            run('sudo yum -y install ' + self.mantidInstaller)
        except Exception as exc:
            # This reports an error if the same package is already installed
            if 'does not update installed package' in str(exc):
                log("Current version is up-to-date, continuing.\n")
            else:
                raise

    def do_uninstall(self):
        """Removes the rpm package
        """
        package_name = os.path.basename(self.mantidInstaller).split("-")[0]
        run('sudo yum -y erase %s' % package_name)


class DMGInstaller(MantidInstaller):
    """Uses an OS X dmg file to install mantid
    """
    def __init__(self, package_dir, do_install):
        MantidInstaller.__init__(self, package_dir, 'mantid-*.dmg', do_install)
        bin_dir = '/Applications/MantidPlot.app/Contents/MacOS'
        self.mantidPlotPath = bin_dir + '/MantidPlot'
        self.python_cmd = bin_dir + '/mantidpython'
        # only necessary on 10.8 build
        if int(platform.release().split('.')[0]) < 13:
            os.environ['DYLD_LIBRARY_PATH'] = '/Applications/MantidPlot.app/Contents/MacOS'

    def do_install(self):
        """Mounts the dmg and copies the application into the right place.
        """
        p = subprocess.Popen(['hdiutil','attach',self.mantidInstaller],stdin=subprocess.PIPE,stdout=subprocess.PIPE)
        p.stdin.write('yes') # This accepts the GPL
        p.communicate()[0] # This captures (and discards) the GPL text
        mantidInstallerName = os.path.basename(self.mantidInstaller)
        mantidInstallerName = mantidInstallerName.replace('.dmg','')
        run('sudo cp -r /Volumes/'+ mantidInstallerName+'/MantidPlot.app /Applications/' )
        run('hdiutil detach /Volumes/'+ mantidInstallerName+'/')

    def do_uninstall(self):
        run('sudo rm -fr /Applications/MantidPlot.app/')


class CondaInstaller(MantidInstaller):

    python_args = "" # not mantidpython. just normal python

    def __init__(self, package_dir, do_install=True):
        filepattern = "mantid-framework*.tar.bz2"
        MantidInstaller.__init__(self, package_dir, filepattern, do_install)
        package = os.path.basename(self.mantidInstaller)
        bindir = os.path.dirname(sys.executable)
        prefix = os.path.dirname(bindir)
        self.conda_mantid_env_prefix = prefix
        self.mantidPlotPath = None # conda mantid-framework does not include mantidplot
        self.python_cmd = sys.executable

    def do_install(self):
        """Uses gdebi to run the install
        """
        thisdir = os.path.dirname(__file__)
        script = os.path.join(thisdir, 'install_conda_mantid.sh')
        run('%s %s' % (script, self.mantidInstaller))

    def do_uninstall(self):
        """Removes the debian package
        """
        # run('rm -rf %s' % self.conda_mantid_env_prefix)


#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------
# If called as a standalone script then this can be used to install/uninstall
# Mantid
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Commands available: install, uninstall")
    parser.add_argument('command', choices=['install', 'uninstall'], help='command to run')
    parser.add_argument('directory', help='package directory')
    parser.add_argument('-d', '--dump-exe-path', dest='dump_exe_path',
                        help='Filepath to write the full path of the MantidPlot executable')

    options = parser.parse_args()

    package_dir = os.path.abspath(options.directory)
    print("Searching for packages in '%s'" % package_dir)
    installer = get_installer(package_dir)
    if options.command == "install":
        print("Installing package '%s'" % installer.mantidInstaller)
        installer.install()
        if options.dump_exe_path is not None:
            with open(options.dump_exe_path, 'w') as f:
                f.write(installer.mantidPlotPath)
    elif options.command == "uninstall":
        print("Removing package '%s'" % installer.mantidInstaller)
        installer.uninstall()
