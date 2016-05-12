"""*WIKI*
Extracts the IPTS number from a run using FileFinde,findRuns. It returns a string the full path to the IPTS shared folder to allow for saving of files in accessible user folders
*WIKI*"""

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
import os


class GetIPTS(PythonAlgorithm):

    def get_IPTS_Local(self, run):
        runs = range(run-5, run+1)
        file_Str = ''

        while len(runs) > 0:
            try:
                r = runs.pop()
                self.log().notice('Checking IPTS from run: is : %s' % r)
                file_Str = FileFinder.findRuns('SNAP_' + str(r))[0]
                break
            except RuntimeError:
                pass

        if file_Str == '':
            self.log().error('The SNAP run %s is not found in the server - cannot find current IPTS, Choose another run' % run)
            return False
        else:
            start = file_Str.find('IPTS')
            temp = file_Str.find('/', start)
            root = file_Str[0:temp+1]
            return root

    def PyInit(self):
        valid = IntBoundedValidator(lower=0)
        self.declareProperty(name="Run Number", defaultValue=0,
                             direction=Direction.Input,
                             validator=valid,
                             doc="Extracts the IPTS number for a run")

    def PyExec(self):
        run = self.getProperty("Run Number").value
        folder = self.get_IPTS_Local(run)
        self.log().notice('Folder is: %s' % folder)

AlgorithmFactory.subscribe(GetIPTS)
