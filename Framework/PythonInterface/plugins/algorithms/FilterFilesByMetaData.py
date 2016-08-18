#pylint: disable=eval-used
from __future__ import (absolute_import, division, print_function)

import h5py
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *
from mantid import config, logger, mtd

class FilterFilesByMetaData(PythonAlgorithm):

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

    def PyExec(self):
        outputfiles = []
        criteria = self.getPropertyValue('Criteria')
        splitted = criteria.split('$')

        for run in self.getPropertyValue('FileList').replace('+', ',').split(','):
            with h5py.File(run,'r') as nexusfile:
                toeval = ''
                for i in range(len(splitted)):
                        toeval += str(nexusfile.get(splitted[i])[0]) if i % 2 == 1 else splitted[i]
                if eval(toeval):
                    outputfiles.append(run)

        print(','.join(outputfiles))
        self.setPropertyValue('FileList',','.join(outputfiles))

# Register algorithm with Mantid
AlgorithmFactory.subscribe(FilterFilesByMetaData)
