#pylint: disable=eval-used,consider-using-enumerate
from __future__ import (absolute_import, division, print_function)

import h5py
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, logger, mtd

class FindFiles(PythonAlgorithm):

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return 'Filters files by metadata criteria'

    def validateInputs(self):
        issues = dict()
        dollars = self.getPropertyValue('NexusCriteria').count('$')
        if  dollars % 2 != 0 or dollars < 2:
            issues['NexusCriteria'] = 'Make sure the nexus entry name is enclosed with $ sybmols'
        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty('FileList',extensions=['nxs']),doc='List of input files')
        self.declareProperty(name='NexusCriteria',defaultValue='',
                             doc='Logical expresion for metadata criteria using python syntax. '
                                 'Provide full absolute names for nexus entries enclosed with $ symbol from both sides.')
        self.declareProperty(name='Result', defaultValue='', direction=Direction.Output,
                             doc='Comma separated list of the fully resolved file names satisfying the given criteria.')

    def PyExec(self):
        outputfiles = []
        splitted = self.getPropertyValue('NexusCriteria').split('$')

        # for the purpose here + is meaningless, so they will silently replaced with ,
        for run in self.getPropertyValue('FileList').replace('+', ',').split(','):
            with h5py.File(run,'r') as nexusfile:
                toeval = ''
                for i in range(len(splitted)):
                    if i % 2 == 1: # at odd indices will always be the nexus entry names
                        # replace nexus entry names by their values
                        try:
                            toeval += str(nexusfile.get(splitted[i])[0])
                        except TypeError:
                            self.log().warning('Nexus entry %s does not exist in file %s. Skipping the file.\n' \
                                               % (splitted[i],run))
                            toeval = '0' # and not False, since builtins are disabled in eval
                            break
                    else:
                        # keep other portions intact
                        toeval += splitted[i]
                try:
                    if eval(toeval):
                        outputfiles.append(run)
                except (NameError,ValueError,SyntaxError):
                    self.log().error('Invalid syntax, check NexusCriteria.')
                    break

        if not outputfiles:
            self.log().notice('No files where found to satisfy the criteria, check the FileList and/or NexusCriteria')

        self.setPropertyValue('Result',','.join(outputfiles))

# Register algorithm with Mantid
AlgorithmFactory.subscribe(FindFiles)
