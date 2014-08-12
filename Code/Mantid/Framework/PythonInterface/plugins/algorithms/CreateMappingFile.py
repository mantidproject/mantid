from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os

class CreateMappingFile(PythonAlgorithm):

    def category(self):
        return 'Inelastic;PythonAlgorithms'

    def summary(self):
        return ''  #TODO

    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '', FileAction.Save))

        self.declareProperty(name='GroupCount', defaultValue=0,
                doc='Number of goups')

        self.declareProperty(name="SpectraRange", defaultValue="0,1",
                doc="SpectraRange")

    def PyExec(self):
        from mantid import config

        group_filename = self.getPropertyValue('Filename')
        num_groups = int(self.getPropertyValue('GroupCount'))
        spectra_range = self.getPropertyValue('SpectraRange').split(',')

        num_detectors = (int(spectra_range[1]) - int(spectra_range[0])) + 1
        num_spectra = num_detectors / num_groups

        filename = config['defaultsave.directory']
        filename = os.path.join(filename, group_filename)

        handle = open(filename, 'w')
        handle.write(str(num_groups) +  "\n")

        for group in range(0, num_groups):
            group_ind = group * num_spectra + int(spectra_range[0])
            handle.write(str(group + 1) + '\n')
            handle.write(str(num_spectra) +  '\n')
            for spectra in range(1, num_spectra+1):
                spectra_ind = group_ind + spectra - 1
                handle.write(str(spectra_ind).center(4) + ' ')
            handle.write('\n')

        handle.close()

AlgorithmFactory.subscribe(CreateMappingFile)
