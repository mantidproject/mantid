#pylint: disable=eval-used
from __future__ import (absolute_import, division, print_function)

import h5py
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import logger

class FindFiles(PythonAlgorithm):

    _criteria_splitted = []

    def category(self):
        return "DataHandling\\Nexus"

    def summary(self):
        return 'Filters files by metadata criteria'

    def validateInputs(self):
        issues = dict()
        criteria = self.getPropertyValue('NexusCriteria')

        # at least one nexus entry should be specified
        dollars = criteria.count('$')
        if dollars % 2 != 0 or dollars < 2:
            issues['NexusCriteria'] = 'Make sure the nexus entry name is enclosed with $ sybmols'
        else:
            # check if the syntax of criteria is valid by replacing the nexus entries with dummy values
            self._criteria_splitted = criteria.split('$')
            toeval = ''
            for i, item in enumerate(self._criteria_splitted):
                if i % 2 == 1:  # at odd indices will always be the nexus entry names
                    # replace nexus entry names by 0
                    toeval += '0'
                else:
                    # keep other portions intact
                    toeval += item
            try:
                eval(toeval)
            except (NameError, ValueError, SyntaxError):
                issues['NexusCriteria'] = 'Invalid syntax, check NexusCriteria.'

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
        # for the purpose here + is meaningless, so they will be silently replaced with ,
        for run in self.getPropertyValue('FileList').replace('+', ',').split(','):
            with h5py.File(run,'r') as nexusfile:
                toeval = ''
                item = None # for pylint
                for i, item in enumerate(self._criteria_splitted):
                    if i % 2 == 1: # at odd indices will always be the nexus entry names
                        try:
                            if len(nexusfile.get(item).shape) > 1:
                                self.log().warning('Nexus entry %s has more than 1 dimensions in file %s.'
                                                   'Skipping the file.' % (item,run))
                                toeval = '0'
                                break

                            # replace entry name by it's value
                            value = nexusfile.get(item)[0]

                            if isinstance(value,str):
                                # string value, need to qoute for eval
                                toeval += '\"'+ value + '\"'
                            else:
                                toeval += str(value)

                        except (TypeError,AttributeError):
                            self.log().warning('Nexus entry %s does not exist in file %s. Skipping the file.' % (item,run))
                            toeval = '0'
                            break
                    else:
                        # keep other portions intact
                        toeval += item
                self.log().debug('Expression to be evaluated for file %s :\n %s' % (run, toeval))
                try:
                    if eval(toeval):
                        outputfiles.append(run)
                except (NameError,ValueError,SyntaxError):
                    # even if syntax is validated, eval can still throw, since
                    # the nexus entry value itself can be spurious for a given file
                    self.log().warning('Invalid value for the nexus entry %s in file %s. Skipping the file.' % (item, run))

        if not outputfiles:
            self.log().notice('No files where found to satisfy the criteria, check the FileList and/or NexusCriteria')

        self.setPropertyValue('Result',','.join(outputfiles))

# Register algorithm with Mantid
AlgorithmFactory.subscribe(FindFiles)
