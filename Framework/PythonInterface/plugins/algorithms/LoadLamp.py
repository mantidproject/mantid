from __future__ import (absolute_import, division, print_function)
from mantid.api import FileProperty, WorkspaceProperty, PythonAlgorithm, AlgorithmFactory, FileAction
from mantid.kernel import Direction
from mantid.simpleapi import CreateWorkspace, AddSampleLogMultiple

import numpy
import h5py


class LoadLamp(PythonAlgorithm):
    def name(self):
        return 'LoadLamp'

    def category(self):
        return 'DataHandling\\Nexus'

    def summary(self):
        return 'Loads HDF files exported from LAMP program at the ILL'

    def PyInit(self):
        self.declareProperty(FileProperty(name="InputFile", defaultValue="", action=FileAction.Load, extensions=["hdf"]))
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output))

    def PyExec(self):
        input_file = self.getProperty("InputFile").value
        output_ws = self.getPropertyValue("OutputWorkspace")
        logs = ''

        with h5py.File(input_file, 'r') as hf:
            data = numpy.array(hf.get('entry1/data1/DATA'), dtype='float')
            if data.ndim > 2:
                raise RuntimeError('Data with more than 2 dimensions are not supported.')
            errors = numpy.array(hf.get('entry1/data1/errors'), dtype='float')
            x = numpy.array(hf.get('entry1/data1/X'), dtype='float')
            if "entry1/data1/PARAMETERS" in hf:
                logs = str(hf.get('entry1/data1/PARAMETERS')[0])
            y = numpy.array([0])
            nspec = 1
            if data.ndim == 2:
                y = numpy.array(hf.get('entry1/data1/Y'), dtype='float')
                nspec = data.shape[0]
                if x.ndim == 1:
                    x = numpy.tile(x, nspec)

        CreateWorkspace(DataX=x, DataY=data, DataE=errors, NSpec=nspec, VerticalAxisUnit='Label',
                        VerticalAxisValues=y, OutputWorkspace=output_ws)

        if logs:
            log_names = []
            log_values = []
            for log in logs.split('\n'):
                split = log.strip().split('=')
                if len(split) == 2:
                    name = split[0]
                    value = split[1]
                    if name and value:
                        log_names.append(name)
                        log_values.append(value)
            if log_names:
                try:
                    AddSampleLogMultiple(Workspace=output_ws, LogNames=log_names, LogValues=log_values)
                except RuntimeError as e:
                    self.log().warning('Unable to set the sample logs, reason: '+str(e))

        self.setProperty('OutputWorkspace', output_ws)

AlgorithmFactory.subscribe(LoadLamp)
