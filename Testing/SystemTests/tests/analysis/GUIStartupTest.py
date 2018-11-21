# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
import platform
import subprocess
import systemtesting
import mantid


class GUIStartupTest(systemtesting.MantidSystemTest):
    '''
    This test is supposed to start MantidPlot, and execute a simple script
    If the script is run correctly, the "Hello Mantid" will be printed to stdout
    If the script fails, there will be a fatal error mesage in stderr
    '''

    def __init__(self):
        super(GUIStartupTest, self).__init__()
        #create simple script
        self.script=os.path.join(mantid.config.getString('defaultsave.directory'),'GUIStartupTest_script.py')
        with open(self.script,'w') as f:
            f.write('import mantid\n'
                    'mantid.config["logging.channels.consoleChannel.class"]="StdoutChannel"\n'
                    'print("Hello Mantid")\n')
        #get the mantidplot executable
        current_platform = platform.platform()
        mantid_init_dirname = os.path.dirname(os.path.abspath(mantid.__file__))
        if 'Linux' in current_platform:
            mantid_exe = os.path.join(mantid_init_dirname,"../launch_mantidplot.sh")
        elif 'Darwin' in current_platform:
            mantid_exe = os.path.join(mantid_init_dirname,"../MantidPlot")
        elif 'Windows' in current_platform:
            mantid_exe = 'wscript.exe {}'.format(os.path.join(mantid_init_dirname,"../bin/launch_mantidplot.vbs"))
        else:
            raise RuntimeError("Unknown operating system")
        self.cmd='{0} -xq {1}'.format(mantid_exe,self.script)

    def cleanup(self):
        if os.path.exists(self.script):
            os.remove(self.script)

    def runTest(self):
        # good startup
        p = subprocess.Popen(self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        out, err = p.communicate()
        self.assertEquals(out, b'Hello Mantid\n')
        self.assertFalse(b'Fatal' in err)

        # failing script
        with open(self.script,'a') as f:
            f.write('raise RuntimeError("GUITest")')
        p = subprocess.Popen(self.cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        out, err = p.communicate()
        self.assertTrue(b'Fatal' in err)
