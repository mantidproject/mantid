#pylint: disable=eval-used,consider-using-enumerate
from __future__ import (absolute_import, division, print_function)

import h5py
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, logger, mtd

class FilterFiles(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;Inelastic\\Reduction"

    def summary(self):
        return 'Filters files by metadata criteria'

    def validateInputs(self):
        issues = dict()
        dollars = self.getPropertyValue('Criteria').count('$')
        if  dollars % 2 != 0 or dollars < 2:
            issues['Criteria'] = 'Make sure the nexus entry name is enclosed with $ sybmols'
        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('FileList',extensions=['nxs']),doc='List of files')
        self.declareProperty(name='Criteria',defaultValue='',doc='Logical expresion for metadata criteria')
        self.declareProperty(name='Result', defaultValue='', direction=Direction.Output, doc='Result string')

    def PyExec(self):
        outputfiles = []
        splitted = self.getPropertyValue('Criteria').split('$')

        for run in self.getPropertyValue('FileList').replace('+', ',').split(','):
            with h5py.File(run,'r') as nexusfile:
                toeval = ''
                for i in range(len(splitted)):
                    if i % 2 == 1:
                        # replace nexus entry names by their values
                        try:
                            toeval += str(nexusfile.get(splitted[i])[0])
                        except TypeError:
                            self.log().warning('Nexus entry %s does not exist in file %s. Skipping the file.\n' \
                                               % (splitted[i],run))
                            toeval = 'False'
                            break
                    else:
                        # keep other portions intact
                        toeval += splitted[i]
                try:
                    if eval(toeval):
                        outputfiles.append(run)
                except SyntaxError:
                    self.log().error('Invalid syntax. Please check Criteria.')
                    break

        if not outputfiles:
            self.log().notice('No files where found. Please check FileList and/or Criteria')

        self.setPropertyValue('Result',','.join(outputfiles))

# Register algorithm with Mantid
AlgorithmFactory.subscribe(FilterFiles)
