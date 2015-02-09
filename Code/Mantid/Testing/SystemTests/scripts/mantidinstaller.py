"""Defines classes for handling installation
"""
import platform
import os
import glob
import sys
import subprocess

scriptLog = None

def createScriptLog(path):
    global scriptLog
    scriptLog = open(path,'w')

def stop(installer):
    ''' Save the log, uninstall the package and exit with error code 0 '''
    try:
        installer.uninstall()
    except Exception, exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))
    scriptLog.close()
    sys.exit(0)

def log(txt):
    ''' Write text to the script log file '''
    if txt and len(txt) > 0:
        scriptLog.write(txt)
        if not txt.endswith('\n'):
            scriptLog.write('\n')
        print txt

def failure(installer):
    ''' Report failure of test(s), try to uninstall package and exit with code 1 '''
    try:
        installer.uninstall()
    except Exception, exc:
        log("Could not uninstall package %s: %s" % (installer.mantidInstaller, str(exc)))
        pass

    log('Tests failed')
    print 'Tests failed'
    sys.exit(1)

def scriptfailure(txt, installer=None):
    '''Report failure of this script, try to uninstall package and exit with code 1 '''
    if txt:
        log(txt)
    if installer is not None:
        try:
            installer.uninstall()
        except Exception:
            log("Could not uninstall package %s " % self.mantidInstaller)
    scriptLog.close()
    sys.exit(1)


def get_installer(do_install=True):
    """
    Creates the correct class for the current platform
        @param do_install :: True if installation is to be performed
    """
    system = platform.system()
    if system == 'Windows':
        return NSISInstaller(do_install)
    elif system == 'Linux':
        dist = platform.dist()
        if dist[0] == 'Ubuntu':
            return DebInstaller(do_install)
        elif dist[0] == 'redhat' and (dist[1].startswith('5.') or dist[1].startswith('6.')):
            return RPMInstaller(do_install)
        else:
            scriptfailure('Unknown Linux flavour: %s' % str(dist))
    elif system == 'Darwin':
        return DMGInstaller(do_install)
    else:
        raise scriptfailure("Unsupported platform")

def run(cmd):
    """Run a command in a subprocess"""
    try:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
        out = p.communicate()[0]
        if p.returncode != 0:
            raise Exception('Returned with code '+str(p.returncode)+'\n'+out)
    except Exception,err:
        log('Error in subprocess %s:\n' % str(err))
        raise
    log(out)
    return out
    

class MantidInstaller(object):
    """
    Base-class for installer objects
    """
    mantidInstaller = None
    mantidPlotPath = None
    no_uninstall = False
    python_cmd = "python"

    def __init__(self, do_install, filepattern):
        """Initialized with a pattern to 
        find a path to an installer
        """
        if not do_install:
            return
        # Glob for packages
        matches = glob.glob(os.path.abspath(filepattern))
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
            log("Using installer " + self.mantidInstaller)
        else:
            raise RuntimeError('Unable to find installer package in "%s"' % os.getcwd())

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

    def __init__(self, do_install):
        MantidInstaller.__init__(self, do_install, 'Mantid-*-win*.exe')
        self.mantidPlotPath = 'C:/MantidInstall/bin/MantidPlot.exe'
        self.python_cmd = "C:/MantidInstall/bin/python.exe"
        
    def do_install(self):
        """
            The NSIS installer spawns a new process and returns immediately.
            We use the start command with the /WAIT option to make it stay around
            until completion.
            The chained "&& exit 1" ensures that if the return code of the
            installer > 0 then the resulting start process exits with a return code
            of 1 so we can pick this up as a failure
        """        
        run('start "Installer" /wait ' + self.mantidInstaller + ' /S')

    def do_uninstall(self):
        "Runs the uninstall exe"
        uninstall_path = 'C:/MantidInstall/Uninstall.exe'
        run('start "Uninstaller" /wait ' + uninstall_path + ' /S')

class DebInstaller(MantidInstaller):
    """Uses a deb package to install mantid
    """

    def __init__(self, do_install):
        MantidInstaller.__init__(self, do_install, 'mantid*.deb')
        package = os.path.basename(self.mantidInstaller)
        if 'mantidnightly' in package:
            self.mantidPlotPath = '/opt/mantidnightly/bin/MantidPlot'
        elif 'mantidunstable' in package:
            self.mantidPlotPath = '/opt/mantidunstable/bin/MantidPlot'
        else:
            self.mantidPlotPath = '/opt/Mantid/bin/MantidPlot'
        
    def do_install(self):
        """Uses gdebi to run the install
        """
        run('sudo gdebi -n ' + self.mantidInstaller)

    def do_uninstall(self):
        """Removes the debian package
        """
        package_name = os.path.basename(self.mantidInstaller).split("_")[0]
        run('sudo dpkg --purge %s' % package_name)

class RPMInstaller(MantidInstaller):
    """Uses a rpm package to install mantid
    """

    def __init__(self, do_install):
        MantidInstaller.__init__(self, do_install, 'mantid*.rpm')
        package = os.path.basename(self.mantidInstaller)
        if 'mantidnightly' in package:
            self.mantidPlotPath = '/opt/mantidnightly/bin/MantidPlot'
        elif 'mantidunstable' in package:
            self.mantidPlotPath = '/opt/mantidunstable/bin/MantidPlot'
        else:
            self.mantidPlotPath = '/opt/Mantid/bin/MantidPlot'
        
    def do_install(self):
        """Uses yum to run the install. Current user must be in sudoers
        """
        try:
            run('sudo yum -y install ' + self.mantidInstaller)
        except Exception, exc:
            # This reports an error if the same package is already installed
            if 'is already installed' in str(exc):
                log("Current version is up-to-date, continuing.\n")
                pass
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
    def __init__(self, do_install):
        MantidInstaller.__init__(self, do_install, 'mantid-*.dmg')
        self.mantidPlotPath = '/Applications/MantidPlot.app/Contents/MacOS/MantidPlot'
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
