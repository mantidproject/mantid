# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
import subprocess
import systemtesting
import mantid


class GUIStartupTest(systemtesting.MantidSystemTest):

    def __init__(self):
        super(GUIStartupTest, self).__init__()
        #create simple script
        self.script=os.path.join(mantid.config.getString('defaultsave.directory'),'GUIStartupTest_script.py')
        with open(self.script,'w') as f:
            f.write('import mantid\n'
                    'mantid.config["logging.channels.consoleChannel.class"]="StdoutChannel"\n'
                    'print("Hello Mantid")\n')

    def skipTests(self):
        #we rely on finding launch_mantidplot.sh
        import platform
        return "Linux" not in platform.platform()

    def cleanup(self):
        if os.path.exists(self.script):
            os.remove(self.script)

    def runTest(self):
        mantid_exe = os.path.join(os.path.dirname(mantid.__file__),"../launch_mantidplot.sh")
        cmd='{0} -xq {1}'.format(mantid_exe,self.script)
        # good startup
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        out, err = p.communicate()

        self.assertEquals(out, b'Hello Mantid\n')
        self.assertFalse(b'Fatal' in err)

        # failing script
        with open(self.script,'a') as f:
            f.write('raise RuntimeError("GUITest")')
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        out, err = p.communicate()
        self.assertEquals(out, b'Hello Mantid\n')
        self.assertTrue(b'Fatal' in err)
