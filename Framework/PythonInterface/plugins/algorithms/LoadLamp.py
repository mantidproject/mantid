from __future__ import (absolute_import, division, print_function)
from mantid.api import FileProperty, WorkspaceProperty, PythonAlgorithm, NumericAxis, AlgorithmFactory, FileAction
from mantid.kernel import Direction
from mantid.simpleapi import CreateWorkspace, AddSampleLogMultiple, mtd

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

        with h5py.File(input_file, 'r') as hf:
            DATA = numpy.array(hf.get('entry1/data1/DATA'), dtype='float')
            if len(DATA.shape) > 2:
                raise RuntimeError('Data with more than 2 dimensions are not supported.')
            E = numpy.array(hf.get('entry1/data1/errors'), dtype='float')
            X = numpy.array(hf.get('entry1/data1/X'), dtype='float')
            LOGS = str(hf.get('entry1/data1/PARAMETERS')[0])
            if len(DATA.shape) == 2:
                Y = numpy.array(hf.get('entry1/data1/Y'), dtype='float')

        nspec = 1
        y_axis = None
        if len(DATA.shape) == 2:
            nspec = DATA.shape[0]
            if len(X.shape) == 1:
                X = numpy.tile(X, nspec)
            y_axis = NumericAxis.create(nspec)
            for i in range(nspec):
                y_axis.setValue(i, Y[i])

        CreateWorkspace(DataX=X, DataY=DATA, DataE=E, NSpec=nspec, OutputWorkspace=output_ws)
        if len(DATA.shape) == 2:
            mtd[output_ws].replaceAxis(1, y_axis)

        log_names = []
        log_values = []
        for log in LOGS.split('\n'):
            split = log.strip().split('=')
            if len(split) == 2:
                name = split[0]
                value = split[1]
                if name and value:
                    log_names.append(name)
                    log_values.append(value)
        AddSampleLogMultiple(Workspace=output_ws, LogNames=log_names, LogValues=log_values, ParseType=False)

        self.setProperty('OutputWorkspace', output_ws)

AlgorithmFactory.subscribe(LoadLamp)
