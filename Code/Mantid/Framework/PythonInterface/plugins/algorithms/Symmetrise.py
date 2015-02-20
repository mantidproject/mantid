#pylint: disable=no-init,invalid-name
from mantid import logger, mtd
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, ITableWorkspaceProperty, PropertyMode
from mantid.kernel import Direction, IntArrayProperty
from mantid.simpleapi import CreateWorkspace, CopyLogs, CopySample, CopyInstrumentParameters, SaveNexusProcessed, CreateEmptyTableWorkspace, RenameWorkspace

import math
import os.path
import numpy as np


class Symmetrise(PythonAlgorithm):

    def category(self):
        return 'PythonAlgorithms'


    def summary(self):
        return 'Make asymmetric workspace data symmetric.'


    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('Sample', '', Direction.Input),
                             doc='Sample to run with')

        self.declareProperty(IntArrayProperty(name='SpectraRange'),
                             doc='Range of spectra to symmetrise (defaults to entire range if not set)')

        self.declareProperty('XMin', 0.0, doc='X value marking lower limit of curve to copy')
        self.declareProperty('XMax', 0.0, doc='X value marking upper limit of curve to copy')

        self.declareProperty('Plot', defaultValue=False,
                             doc='Switch plotting Off/On')
        self.declareProperty('Save', defaultValue=False,
                             doc='Switch saving result to nxs file Off/On')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',\
                             Direction.Output), doc='Name to call the output workspace.')

        self.declareProperty(ITableWorkspaceProperty('OutputPropertiesTable', '',\
                             Direction.Output, PropertyMode.Optional), doc='Name to call the properties output table workspace.')


    def PyExec(self):
        from IndirectCommon import StartTime, EndTime

        StartTime('Symmetrise')
        self._setup()
        temp_ws_name = '__symm_temp'

        # The number of spectra that will actually be changed
        num_symm_spectra = self._spectra_range[1] - self._spectra_range[0] + 1

        # Find the smallest data array in the first spectra
        len_x = len(mtd[self._sample].readX(0))
        len_y = len(mtd[self._sample].readY(0))
        len_e = len(mtd[self._sample].readE(0))
        sample_array_len = min(len_x, len_y, len_e)

        sample_x = mtd[self._sample].readX(0)

        # Get slice bounds of array
        try:
            self._calculate_array_points(sample_x, sample_array_len)
        except Exception as e:
            raise RuntimeError('Failed to calculate array slice boundaries: %s' % e.message)

        max_sample_index = sample_array_len - 1
        centre_range_len = self._positive_min_index + self._negative_min_index
        positive_diff_range_len = max_sample_index - self._positive_max_index

        output_cut_index = max_sample_index - self._positive_min_index - positive_diff_range_len - 1
        new_array_len = 2 * max_sample_index - centre_range_len - 2 * positive_diff_range_len - 1

        logger.information('Sample array length = %d' % sample_array_len)

        logger.information('Positive X min at i=%d, x=%f'
                           % (self._positive_min_index, sample_x[self._positive_min_index]))
        logger.information('Negative X min at i=%d, x=%f'
                           % (self._negative_min_index, sample_x[self._negative_min_index]))

        logger.information('Positive X max at i=%d, x=%f'
                           % (self._positive_max_index, sample_x[self._positive_max_index]))

        logger.information('New array length = %d' % new_array_len)
        logger.information('Output array LR split index = %d' % output_cut_index)

        x_unit = mtd[self._sample].getAxis(0).getUnit().unitID()
        v_unit = mtd[self._sample].getAxis(1).getUnit().unitID()
        v_axis_data = mtd[self._sample].getAxis(1).extractValues()

        # Take the values we need from the original vertical axis
        min_spectrum_index = mtd[self._sample].getIndexFromSpectrumNumber(int(self._spectra_range[0]))
        max_spectrum_index = mtd[self._sample].getIndexFromSpectrumNumber(int(self._spectra_range[1]))
        new_v_axis_data = v_axis_data[min_spectrum_index:max_spectrum_index + 1]

        # Create an empty workspace with enough storage for the new data
        zeros = np.zeros(new_array_len * num_symm_spectra)
        CreateWorkspace(OutputWorkspace=temp_ws_name,
                        DataX=zeros, DataY=zeros, DataE=zeros,
                        NSpec=int(num_symm_spectra),
                        VerticalAxisUnit=v_unit, VerticalAxisValues=new_v_axis_data,
                        UnitX=x_unit)

        # Copy logs and properties from sample workspace
        CopyLogs(InputWorkspace=self._sample, OutputWorkspace=temp_ws_name)
        CopyInstrumentParameters(InputWorkspace=self._sample, OutputWorkspace=temp_ws_name)

        # For each spectrum copy positive values to the negative
        output_spectrum_index = 0
        for spectrum_no in range(self._spectra_range[0], self._spectra_range[1] + 1):
            # Get index of original spectra
            spectrum_index = mtd[self._sample].getIndexFromSpectrumNumber(spectrum_no)

            # Strip any additional array cells
            x_in = mtd[self._sample].readX(spectrum_index)[:sample_array_len]
            y_in = mtd[self._sample].readY(spectrum_index)[:sample_array_len]
            e_in = mtd[self._sample].readE(spectrum_index)[:sample_array_len]

            # Get some zeroed data to overwrite with copies from sample
            x_out = np.zeros(new_array_len)
            y_out = np.zeros(new_array_len)
            e_out = np.zeros(new_array_len)

            # Left hand side (reflected)
            x_out[:output_cut_index] = -x_in[self._positive_max_index - 1:self._positive_min_index:-1]
            y_out[:output_cut_index] = y_in[self._positive_max_index - 1:self._positive_min_index:-1]
            e_out[:output_cut_index] = e_in[self._positive_max_index - 1:self._positive_min_index:-1]

            # Right hand side (copied)
            x_out[output_cut_index:] = x_in[self._negative_min_index:self._positive_max_index]
            y_out[output_cut_index:] = y_in[self._negative_min_index:self._positive_max_index]
            e_out[output_cut_index:] = e_in[self._negative_min_index:self._positive_max_index]

            # Set output spectrum data
            mtd[temp_ws_name].setX(output_spectrum_index, x_out)
            mtd[temp_ws_name].setY(output_spectrum_index, y_out)
            mtd[temp_ws_name].setE(output_spectrum_index, e_out)

            # Set output spectrum number
            mtd[temp_ws_name].getSpectrum(output_spectrum_index).setSpectrumNo(spectrum_no)
            output_spectrum_index += 1

            logger.information('Symmetrise spectrum %d' % spectrum_no)

        RenameWorkspace(InputWorkspace=temp_ws_name, OutputWorkspace=self._output_workspace)

        if self._save:
            self._save_output()

        if self._plot:
            self._plot_output()

        if self._props_output_workspace != '':
            self._generate_props_table()

        self.setProperty('OutputWorkspace', self._output_workspace)

        EndTime('Symmetrise')


    def validateInputs(self):
        """
        Checks for invalid input properties.
        """
        from IndirectCommon import CheckHistZero
        issues = dict()

        input_workspace_name = self.getPropertyValue('Sample')

        # Validate spectra range
        spectra_range = self.getProperty('SpectraRange').value
        if len(spectra_range) != 0 and len(spectra_range) != 2:
            issues['SpectraRange'] = 'Must be in format "spec_min,spec_max"'

        if len(spectra_range) == 2:
            spec_min = spectra_range[0]
            spec_max = spectra_range[1]

            num_sample_spectra, _ = CheckHistZero(input_workspace_name)
            min_spectra_number = mtd[input_workspace_name].getSpectrum(0).getSpectrumNo()
            max_spectra_number = mtd[input_workspace_name].getSpectrum(num_sample_spectra - 1).getSpectrumNo()

            if spec_min < min_spectra_number:
                issues['SpectraRange'] = 'Minimum spectra must be greater than or equal to %d' % min_spectra_number

            if spec_max > max_spectra_number:
                issues['SpectraRange'] = 'Maximum spectra must be less than or equal to %d' % max_spectra_number

            if spec_max < spec_min:
                issues['SpectraRange'] = 'Minimum spectra must be smaller than maximum spectra'

        # Validate X range
        x_min = self.getProperty('XMin').value
        if x_min < -1e-5:
            issues['XMin'] = 'XMin must be greater than or equal to zero'

        x_max = self.getProperty('XMax').value
        if x_max < 1e-5:
            issues['XMax'] = 'XMax must be greater than zero'

        if math.fabs(x_max - x_min) < 1e-5:
            issues['XMin'] = 'X range is close to zero'
            issues['XMax'] = 'X range is close to zero'

        if x_max < x_min:
            issues['XMin'] = 'XMin must be less than XMax'
            issues['XMax'] = 'XMax must be greater than XMin'

        # Valudate X range against workspace X range
        sample_x = mtd[input_workspace_name].readX(0)
        sample_x_min = sample_x.min()
        sample_x_max = sample_x.max()

        if x_max > sample_x_max:
            issues['XMax'] = 'XMax value (%f) is greater than largest X value (%f)' % (x_max, sample_x_max)

        if -x_min < sample_x_min:
            issues['XMin'] = 'Negative XMin value (%f) is less than smallest X value (%f)' % (-x_min, sample_x_min)

        return issues


    def _setup(self):
        """
        Get the algorithm properties and validate them.
        """
        from IndirectCommon import CheckHistZero

        self._sample = self.getPropertyValue('Sample')

        self._x_min = math.fabs(self.getProperty('XMin').value)
        self._x_max = math.fabs(self.getProperty('XMax').value)

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        self._spectra_range = self.getProperty('SpectraRange').value
        # If the user did not enter a spectra range, use the spectra range of the workspace
        if len(self._spectra_range) == 0:
            num_sample_spectra, _ = CheckHistZero(self._sample)
            min_spectra_number = mtd[self._sample].getSpectrum(0).getSpectrumNo()
            max_spectra_number = mtd[self._sample].getSpectrum(num_sample_spectra - 1).getSpectrumNo()
            self._spectra_range = [min_spectra_number, max_spectra_number]

        self._output_workspace = self.getPropertyValue('OutputWorkspace')
        self._props_output_workspace = self.getPropertyValue('OutputPropertiesTable')


    def _calculate_array_points(self, sample_x, sample_array_len):
        """
        Finds the points in the array that match the cut points.

        @param sample_x - Sample X axis data
        @param sample_array_len - Lengh of data array for sample data
        """
        delta_x = sample_x[1] - sample_x[0]

        # Find array index of negative XMin
        negative_min_diff = np.absolute(sample_x + self._x_min)
        self._negative_min_index = np.where(negative_min_diff < delta_x)[0][-1]
        self._check_bounds(self._negative_min_index, sample_array_len, label='Negative')

        # Find array index of positive XMin
        positive_min_diff = np.absolute(sample_x + sample_x[self._negative_min_index])
        self._positive_min_index = np.where(positive_min_diff < delta_x)[0][-1]
        self._check_bounds(self._positive_min_index, sample_array_len, label='Positive')

        # Find array index of positive XMax
        positive_max_diff = np.absolute(sample_x - self._x_max)
        self._positive_max_index = np.where(positive_max_diff < delta_x)[0][-1]
        if self._positive_max_index == sample_array_len:
            self._positive_max_index -= 1
        self._check_bounds(self._positive_max_index, sample_array_len, label='Positive')


    def _check_bounds(self, index, num_pts, label=''):
        """
        Check if the index falls within the bounds of the x range.
        Throws a ValueError if the x point falls outside of the range.

        @param index  - value of the index within the x range.
        @param num_pts - total number of points in the range.
        @param label - label to call the point if an error is thrown.
        """
        if index < 0:
            raise ValueError('%s point %d < 0' % (label, index))
        elif index >= num_pts:
            raise ValueError('%s point %d > %d' % (label, index, num_pts))


    def _generate_props_table(self):
        """
        Creates a table workspace with values calculated in algorithm.
        """
        props_table = CreateEmptyTableWorkspace(OutputWorkspace=self._props_output_workspace)

        props_table.addColumn('int', 'NegativeXMinIndex')
        props_table.addColumn('int', 'PositiveXMinIndex')
        props_table.addColumn('int', 'PositiveXMaxIndex')

        props_table.addRow([int(self._negative_min_index), int(self._positive_min_index), int(self._positive_max_index)])

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

        logger.information('Output file : ' + file_path)


    def _plot_output(self):
        """
        Plot the first spectrum of the input and output workspace together.
        """
        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()

        mtd_plot.plotSpectrum([self._sample, self._output_workspace], 0)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Symmetrise)
