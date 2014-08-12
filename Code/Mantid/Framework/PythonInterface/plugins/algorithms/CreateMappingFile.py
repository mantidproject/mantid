from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os


class CreateMappingFile(PythonAlgorithm):

    def category(self):
        return 'Inelastic;PythonAlgorithms'

    def summary(self):
        return 'Create a mapping file for the Group Detectors algorithm.'

    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '', FileAction.Save, ['map']),
            doc='Filename to save generated mapping as')

        self.declareProperty(name='GroupCount', defaultValue=0, validator=IntMandatoryValidator(),
            doc='Number of groups in mapping')

        self.declareProperty(IntArrayProperty(name="SpectraRange", values=[0, 1],
            validator=IntArrayMandatoryValidator()),
            doc="Range of spectra in mapping")

    def PyExec(self):
        from mantid import config

        group_filename = self.getPropertyValue('Filename')
        num_groups = self.getProperty('GroupCount').value
        spectra_range = self.getProperty('SpectraRange').value

        num_detectors = (spectra_range[1] - spectra_range[0]) + 1
        num_spectra = num_detectors / num_groups

        filename = config['defaultsave.directory']
        filename = os.path.join(filename, group_filename)

        handle = open(filename, 'w')
        handle.write(str(num_groups) +  "\n")

        for group in range(0, num_groups):
            group_ind = group * num_spectra + spectra_range[0]
            handle.write(str(group + 1) + '\n')
            handle.write(str(num_spectra) +  '\n')
            for spectra in range(1, num_spectra+1):
                spectra_ind = group_ind + spectra - 1
                handle.write(str(spectra_ind).center(4) + ' ')
            handle.write('\n')

        handle.close()

AlgorithmFactory.subscribe(CreateMappingFile)
