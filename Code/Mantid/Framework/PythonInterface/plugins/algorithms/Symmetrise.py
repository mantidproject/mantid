from mantid import logger, mtd
from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, PropertyMode
from mantid.kernel import Direction
from mantid.simpleapi import CreateWorkspace, CopyLogs, CopySample, CopyInstrumentParameters, SaveNexusProcessed, CreateEmptyTableWorkspace

import math
import os.path
import numpy as np


class Symmetrise(PythonAlgorithm):

    def category(self):
        return 'Workflow\\MIDAS;PythonAlgorithms'

    def summary(self):
        return 'Takes an asymmetric S(Q,w) and makes it symmetric'

    def PyInit(self):
        self.declareProperty(WorkspaceProperty('Sample', '', Direction.Input),
                             doc='Sample to run with')
        self.declareProperty('XCut', 0.0, doc='X cut off value')

        self.declareProperty('Verbose', defaultValue=False,
                             doc='Switch verbose output Off/On')
        self.declareProperty('Plot', defaultValue=False,
                             doc='Switch plotting Off/On')
        self.declareProperty('Save', defaultValue=False,
                             doc='Switch saving result to nxs file Off/On')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                             Direction.Output), doc='Name to call the output workspace.')

        self.declareProperty(WorkspaceProperty('OutputPropertiesTable', '',
                             Direction.Output, PropertyMode.Optional), doc='Name to call the properties output table workspace.')

    def PyExec(self):
        from IndirectCommon import CheckHistZero, StartTime, EndTime

        StartTime('Symmetrise')
        self._setup()
        num_spectra, _ = CheckHistZero(self._sample)
        sample_x = mtd[self._sample].readX(0)

        if math.fabs(self._x_cut) < 1e-5:
            raise ValueError('XCut point is Zero')

        # Find the smallest data array in the first spectra
        len_x = len(mtd[self._sample].readX(0))
        len_y = len(mtd[self._sample].readY(0))
        len_e = len(mtd[self._sample].readE(0))
        sample_array_len = min(len_x, len_y, len_e) - 1

        delta_x = sample_x[1] - sample_x[0]

        # Find array index of negative XCut
        negative_diff = np.absolute(sample_x + self._x_cut)
        self._negative_index = np.where(negative_diff < delta_x)[0][-1]
        self._check_bounds(self._negative_index, sample_array_len, label='Negative')

        # FInd array index of positive XCut
        positive_diff = np.absolute(sample_x + sample_x[self._negative_index])
        self._positive_index = np.where(positive_diff < delta_x)[0][-1]
        self._check_bounds(self._positive_index, sample_array_len, label='Positive')

        # Calculate number of elements neede dfor new array (per spectra)
        new_array_len = 2 * sample_array_len - (self._positive_index + self._negative_index) + 1

        if self._verbose:
            logger.notice('No. points = %d' % sample_array_len)
            logger.notice('Negative at i=%d, x=%f'
                          % (self._negative_index, sample_x[self._negative_index]))
            logger.notice('Positive at i=%d, x=%f'
                          % (self._positive_index, sample_x[self._positive_index]))
            logger.notice('New array size = %d' % new_array_len)

        # Create an empty workspace with enough storage for the new data
        zeros = np.zeros(new_array_len * num_spectra)
        CreateWorkspace(OutputWorkspace=self._output_workspace,
                        DataX=zeros, DataY=zeros, DataE=zeros,
                        NSpec=num_spectra)

        # Copy logs and properties from sample workspace
        CopyLogs(InputWorkspace=self._sample, OutputWorkspace=self._output_workspace)
        CopyInstrumentParameters(InputWorkspace=self._sample, OutputWorkspace=self._output_workspace)
        # CopySample(InputWorkspace=self._sample, OutputWorkspace=self._output_workspace)

        # For each spectrum copy positive values to the negative
        for index in xrange(num_spectra):
            # Strip any additional array cells
            x_in = mtd[self._sample].readX(index)[:sample_array_len + 1]
            y_in = mtd[self._sample].readY(index)[:sample_array_len + 1]
            e_in = mtd[self._sample].readE(index)[:sample_array_len + 1]

            # Get some zeroed data to overwrite with copies from sample
            x_out = np.zeros(new_array_len)
            y_out = np.zeros(new_array_len)
            e_out = np.zeros(new_array_len)

            # Left hand side of cut
            x_out[:sample_array_len - self._positive_index] = -x_in[sample_array_len:self._positive_index:-1]
            y_out[:sample_array_len - self._positive_index] = y_in[sample_array_len:self._positive_index:-1]
            e_out[:sample_array_len - self._positive_index] = e_in[sample_array_len:self._positive_index:-1]

            # Right hand side of cut
            x_out[sample_array_len - self._positive_index:] = x_in[self._negative_index:]
            y_out[sample_array_len - self._positive_index:] = y_in[self._negative_index:]
            e_out[sample_array_len - self._positive_index:] = e_in[self._negative_index:]

            mtd[self._output_workspace].setX(index, x_out)
            mtd[self._output_workspace].setY(index, y_out)
            mtd[self._output_workspace].setE(index, e_out)

            logger.information('Spectra %d out of %d done'
                               % (index + 1, num_spectra))

        if self._save:
            self._save_output()

        if self._plot:
            self._plot_output()

        if self._props_output_workspace != '':
            self._generate_props_table()

        self.setProperty('OutputWorkspace', self._output_workspace)

        EndTime('Symmetrise')

    def _setup(self):
        """
        Get the algorithm properties.
        """
        self._sample = self.getPropertyValue('Sample')
        self._x_cut = self.getProperty('XCut').value

        self._verbose = self.getProperty('Verbose').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        self._output_workspace = self.getPropertyValue('OutputWorkspace')
        self._props_output_workspace = self.getPropertyValue('OutputPropertiesTable')

    def _check_bounds(self, index, num_pts, label=''):
        """
        Check if the index falls within the bounds of the x range.
        Throws a ValueError if the x point falls outside of the range.

        @param index  - value of the index within the x range.
        @param num_pts - total number of points in the range.
        @param label - label to call the point if an error is thrown.
        """
        if index <= 0:
            raise ValueError('%s point %d < 0' % (label, index))
        elif index >= num_pts:
            raise ValueError('%s point %d > %d' % (label, index, num_pts))

    def _generate_props_table(self):
        """
        Creates a table workspace with values calculated in algorithm.
        """
        props_table = CreateEmptyTableWorkspace(OutputWorkspace=self._props_output_workspace)

        props_table.addColumn('int', 'NegativeCutIndex')
        props_table.addColumn('int', 'PositiveCutIndex')

        props_table.addRow([self._negative_index, self._positive_index])

        self.setProperty('OutputPropertiesTable', self._props_output_workspace)

    def _save_output(self):
        """
        Save the output workspace to the user's default working directory
        """
        from IndirectCommon import getDefaultWorkingDirectory
        workdir = getDefaultWorkingDirectory()
        file_path = os.path.join(workdir, self._output_workspace + '.nxs')
        SaveNexusProcessed(InputWorkspace=self._output_workspace,
                           Filename=file_path)

        if self._verbose:
            logger.notice('Output file : ' + file_path)

    def _plot_output(self):
        """
        Plot the first spectrum of the input and output workspace together.
        """
        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()
        mtd_plot.plotSpectrum([self._sample, self._output_workspace], 0)

# Register algorithm with Mantid
AlgorithmFactory.subscribe(Symmetrise)
