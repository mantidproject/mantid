# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
"""Finds a package, installs it and runs the tests against it.
"""
import argparse
import os
import subprocess

from mantidinstaller import (createScriptLog, log, stop, failure, scriptfailure,
                             get_installer, run)

THIS_MODULE_DIR = os.path.dirname(os.path.realpath(__file__))
SAVE_DIR_LIST_PATH = os.path.join(THIS_MODULE_DIR, "defaultsave-directory.txt")

parser = argparse.ArgumentParser()
parser.add_argument('-d', dest='package_dir', metavar='directory', default=os.getcwd(),
                    help='Directory to look for packages. Defaults to current working directory')
parser.add_argument('-j', '--parallel', dest='ncores', type=int, default=1,
                    help='The number of instances to run in parallel, like the -j option in ctest.')
parser.add_argument('-n', dest='doInstall', action='store_false',
                    help='Run tests without installing Mantid (it must be already installed)')
parser.add_argument('-o', dest='out2stdout', action='store_true',
                    help='Output to the screen instead of log files')
parser.add_argument('-R', dest='test_regex', metavar='regexp', default=None,
                    help='Optionally only run the test matched by the regex')
parser.add_argument("-E", dest="test_exclude", metavar="exclude", default=None,
                    help="String specifying which tests to not run")
parser.add_argument('--archivesearch', dest='archivesearch', action='store_true',
                    help='Turn on archive search for file finder')
parser.add_argument('--exclude-in-pull-requests', dest="exclude_in_pr_builds",action="store_true",
                    help="Skip tests that are not run in pull request builds")
log_levels = ['error', 'warning', 'notice', 'information', 'debug']
parser.add_argument('-l', dest='log_level', metavar='level', default='information',
                    choices=log_levels, help='Log level '+str(log_levels))
options = parser.parse_args()

# Log to the configured default save directory
with open(SAVE_DIR_LIST_PATH, 'r') as f_handle:
    output_dir = f_handle.read().strip()

createScriptLog(os.path.join(output_dir, "TestScript.log"))
testRunLogPath = os.path.join(output_dir, "test_output.log")
testRunErrPath = os.path.join(output_dir, "test_errors.log")

log('Starting system tests')
log('Searching for packages in ' + options.package_dir)
installer = get_installer(options.package_dir, options.doInstall)

# Install the found package
if options.doInstall:
    log("Installing package '%s'" % installer.mantidInstaller)
    try:
        installer.install()
        log("Application path: %r" % installer.mantidPlotPath)
        installer.no_uninstall = False
    except Exception as err:
        scriptfailure("Installing failed. "+str(err))
else:
    installer.no_uninstall = True

# conda mantid-framework does not have mantid plot. skip these
if not os.environ.get('MANTID_FRAMEWORK_CONDA_SYSTEMTEST'):
    try:
        # Keep hold of the version that was run
        version = run(installer.mantidPlotPath + ' -v')
        version_tested = open(os.path.join(output_dir,'version_tested.log'),'w')
        if version and len(version) > 0:
            version_tested.write(version)
        version_tested.close()
    except Exception as err:
        scriptfailure('Version test failed: '+str(err), installer)

    try:
        # Now get the revision number/git commit ID (remove the leading 'g' that isn't part of it)
        revision = run(installer.mantidPlotPath + ' -r').lstrip('g')
        revision_tested = open(os.path.join(output_dir, 'revision_tested.log'), 'w')
        if revision and len(version) > 0:
            revision_tested.write(revision)
        revision_tested.close()
    except Exception as err:
        scriptfailure('Revision test failed: '+str(err), installer)

log("Running system tests. Log files are: '%s' and '%s'" % (testRunLogPath,testRunErrPath))
try:
    run_test_cmd = '%s %s %s/runSystemTests.py --loglevel=%s --executable="%s" --exec-args="%s"' % \
                (installer.python_cmd, installer.python_args, THIS_MODULE_DIR,
                options.log_level, installer.python_cmd, installer.python_args)
    run_test_cmd += " -j%i --quiet --output-on-failure" % options.ncores
    if options.test_regex is not None:
        run_test_cmd += " -R " + options.test_regex
    if options.test_exclude is not None:
        run_test_cmd += " -E " + options.test_exclude
    if options.archivesearch:
        run_test_cmd += ' --archivesearch'
    if options.exclude_in_pr_builds:
        run_test_cmd += ' --exclude-in-pull-requests'
    if options.out2stdout:
        print("Executing command '{0}'".format(run_test_cmd))
        p = subprocess.Popen(run_test_cmd, shell=True) # no PIPE: print on screen for debugging
        p.wait()
    else:
        p = subprocess.Popen(run_test_cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True)
        out,err = p.communicate() # waits for p to finish
        testsRunLog = open(testRunLogPath,'w')
        if out:
            testsRunLog.write(out)
        testsRunLog.close()
        testsRunErr = open(testRunErrPath,'w')
        if err:
            testsRunErr.write(err)
        testsRunErr.close()
    if p.returncode != 0:
        failure(installer)
except Exception as exc:
    scriptfailure(str(exc),installer)
except:
    failure(installer)

# Test run completed successfully
stop(installer)
