#pylint: disable=no-init,invalid-name,too-many-locals,too-many-lines
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np
import re
import os.path
import math

PEAK_WIDTH_ENERGY_FLAG = 'energy'

#pylint: disable=too-many-instance-attributes
class SimulatedDensityOfStates(PythonAlgorithm):

    _float_regex = None
    _temperature = None
    _bin_width = None
    _spec_type = None
    _peak_func = None
    _out_ws_name = None
    _peak_width = None
    _scale = None
    _zero_threshold = None
    _ions = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _calc_partial = None
    _num_ions = None
    _num_branches = None

#----------------------------------------------------------------------------------------

    def category(self):
        return "Simulation"

#----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates phonon densities of states, Raman and IR spectrum."

#----------------------------------------------------------------------------------------

    def PyInit(self):
        # Declare properties
        self.declareProperty(FileProperty('CASTEPFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions = ["castep"]),
                             doc='Filename of the CASTEP file.')

        self.declareProperty(FileProperty('PHONONFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions = ["phonon"]),
                             doc='Filename of the PHONON file.')

        self.declareProperty(name='Function',defaultValue='Gaussian',
                             validator=StringListValidator(['Gaussian', 'Lorentzian']),
                             doc="Type of function to fit to peaks.")

        self.declareProperty(name='PeakWidth', defaultValue='10.0',
                             doc='Set Gaussian/Lorentzian FWHM for broadening. Default is 10')

        self.declareProperty(name='SpectrumType', defaultValue='DOS',
                             validator=StringListValidator(['IonTable', 'DOS', 'IR_Active', 'Raman_Active', 'BondTable']),
                             doc="Type of intensities to extract and model (fundamentals-only) from .phonon.")

        self.declareProperty(name='StickHeight', defaultValue=0.01,
                             doc='Intensity of peaks in stick diagram.')

        self.declareProperty(name='Scale', defaultValue=1.0,
                             doc='Scale the intesity by the given factor. Default is no scaling.')

        self.declareProperty(name='BinWidth', defaultValue=1.0,
                             doc='Set histogram resolution for binning (eV or cm**-1). Default is 1')

        self.declareProperty(name='Temperature', defaultValue=300.0,
                             doc='Temperature to use (in raman spectrum modelling). Default is 300')

        self.declareProperty(name='ZeroThreshold', defaultValue=3.0,
                             doc='Ignore frequencies below the this threshold. Default is 3.0')

        self.declareProperty(StringArrayProperty('Ions', Direction.Input),
                             doc="List of Ions to use to calculate partial density of states."\
                                 "If left blank, total density of states will be calculated")

        self.declareProperty(name='SumContributions', defaultValue=False,
                             doc="Sum the partial density of states into a single workspace.")

        self.declareProperty(name='ScaleByCrossSection', defaultValue='None',
                             validator=StringListValidator(['None', 'Total', 'Incoherent', 'Coherent']),
                             doc="Sum the partial density of states by the scattering cross section.")

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace.")

        # Regex pattern for a floating point number
        self._float_regex = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'

#----------------------------------------------------------------------------------------

    def validateInputs(self):
        """
        Performs input validation.

        Used to ensure the user is requesting a valid mode.
        """
        issues = dict()

        castep_filename = self.getPropertyValue('CASTEPFile')
        phonon_filename = self.getPropertyValue('PHONONFile')

        if castep_filename == '' and phonon_filename == '':
            msg = 'Must have at least one input file'
            issues['CASTEPFile'] = msg
            issues['PHONONFile'] = msg

        spec_type = self.getPropertyValue('SpectrumType')
        sum_contributions = self.getProperty('SumContributions').value
        scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection') != 'None'

        ions = self.getProperty('Ions').value
        calc_partial = len(ions) > 0

        if spec_type == 'IonTable' and phonon_filename == '':
            issues['SpectrumType'] = 'Require a .phonon file for ion table output'

        if spec_type == 'BondAnalysis' and phonon_filename == '' and castep_filename == '':
            issues['SpectrumType'] = 'Require both a .phonon and .castep file for bond analysis'

        if spec_type == 'BondTable' and castep_filename == '':
            issues['SpectrumType'] = 'Require a .castep file for bond table output'

        if spec_type != 'DOS' and calc_partial:
            issues['Ions'] = 'Cannot calculate partial density of states when using %s' % spec_type

        if spec_type != 'DOS' and scale_by_cross_section:
            issues['ScaleByCrossSection'] = 'Cannot scale contributions by cross sections when using %s' % spec_type

        if phonon_filename == '' and scale_by_cross_section:
            issues['ScaleByCrossSection'] = 'Must supply a PHONON file when scaling by cross sections'

        if not calc_partial and sum_contributions:
            issues['SumContributions'] = 'Cannot sum contributions when not calculating partial density of states'

        return issues

#----------------------------------------------------------------------------------------

    #pylint: disable=too-many-branches
    def PyExec(self):
        # Run the algorithm
        self._get_properties()

        castep_filename = self.getPropertyValue('CASTEPFile')
        phonon_filename = self.getPropertyValue('PHONONFile')

        if phonon_filename != '' and self._spec_type != 'BondTable':
            file_data = self._read_data_from_file(phonon_filename)
        elif castep_filename != '':
            file_data = self._read_data_from_file(castep_filename)
        else:
            raise RuntimeError('No valid data file')

        frequencies = file_data['frequencies']
        ir_intensities = file_data['ir_intensities']
        raman_intensities = file_data['raman_intensities']
        weights = file_data['weights']
        eigenvectors = file_data.get('eigenvectors', None)
        ions = file_data.get('ions', None)
        unit_cell = file_data.get('unit_cell', None)

        logger.debug('Unit cell: {0}'.format(unit_cell))

        prog_reporter = Progress(self, 0.0, 1.0, 1)

        # We want to output a table workspace with ion information
        if self._spec_type == 'IonTable':
            ion_table = CreateEmptyTableWorkspace(OutputWorkspace=self._out_ws_name)
            ion_table.addColumn('str', 'Species')
            ion_table.addColumn('int', 'FileIndex')
            ion_table.addColumn('int', 'Number')
            ion_table.addColumn('float', 'FractionalX')
            ion_table.addColumn('float', 'FractionalY')
            ion_table.addColumn('float', 'FractionalZ')
            ion_table.addColumn('float', 'CartesianX')
            ion_table.addColumn('float', 'CartesianY')
            ion_table.addColumn('float', 'CartesianZ')

            self._convert_to_cartesian_coordinates(unit_cell, ions)

            for ion in ions:
                ion_table.addRow([ion['species'],
                                  ion['index'],
                                  ion['bond_number'],
                                  ion['fract_coord'][0],
                                  ion['fract_coord'][1],
                                  ion['fract_coord'][2],
                                  ion['cartesian_coord'][0],
                                  ion['cartesian_coord'][1],
                                  ion['cartesian_coord'][2]])

        # We want to output a table workspace with bond information
        if self._spec_type == 'BondTable':
            bonds = file_data.get('bonds', None)
            if bonds is None or len(bonds) == 0:
                raise RuntimeError('No bonds found in CASTEP file')

            bond_table = CreateEmptyTableWorkspace(OutputWorkspace=self._out_ws_name)
            bond_table.addColumn('str', 'SpeciesA')
            bond_table.addColumn('int', 'NumberA')
            bond_table.addColumn('str', 'SpeciesB')
            bond_table.addColumn('int', 'NumberB')
            bond_table.addColumn('float', 'Length')
            bond_table.addColumn('float', 'Population')

            for bond in bonds:
                bond_table.addRow([bond['atom_a'][0],
                                   bond['atom_a'][1],
                                   bond['atom_b'][0],
                                   bond['atom_b'][1],
                                   bond['length'],
                                   bond['population']])

        # We want to calculate a partial DoS
        elif self._calc_partial and self._spec_type == 'DOS':
            logger.notice('Calculating partial density of states')
            prog_reporter.report('Calculating partial density of states')

            # Build a dictionary of ions that the user cares about
            partial_ions = dict()
            for ion in self._ions:
                partial_ions[ion] = [i['index'] for i in ions if i['species'] == ion]

            partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(partial_ions, frequencies, eigenvectors, weights)

            if self._sum_contributions:
                # Discard the partial workspaces
                for partial_ws in partial_workspaces:
                    DeleteWorkspace(partial_ws)

                # Rename the summed workspace, this will be the output
                RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._out_ws_name)

            else:
                DeleteWorkspace(sum_workspace)

                group = ','.join(partial_workspaces)
                GroupWorkspaces(group, OutputWorkspace=self._out_ws_name)

        # We want to calculate a total DoS with scaled intensities
        elif self._spec_type == 'DOS' and self._scale_by_cross_section != 'None':
            logger.notice('Calculating summed density of states with scaled intensities')
            prog_reporter.report('Calculating density of states')

            # Build a dict of all ions
            all_ions = dict()
            for ion in set([i['species'] for i in ions]):
                all_ions[ion] = [i['index'] for i in ions if i['species'] == ion]

            partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(all_ions, frequencies, eigenvectors, weights)

            # Discard the partial workspaces
            for partial_ws in partial_workspaces:
                DeleteWorkspace(partial_ws)

            # Rename the summed workspace, this will be the output
            RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._out_ws_name)

        # We want to calculate a total DoS without scaled intensities
        elif self._spec_type == 'DOS':
            logger.notice('Calculating summed density of states without scaled intensities')
            prog_reporter.report('Calculating density of states')

            self._compute_DOS(frequencies, np.ones_like(frequencies), weights)
            mtd[self._out_ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._out_ws_name].setYUnitLabel('Intensity')

        # We want to calculate a DoS with IR active
        elif self._spec_type == 'IR_Active':
            if ir_intensities.size == 0:
                raise ValueError('Could not load any IR intensities from file.')

            logger.notice('Calculating IR intensities')
            prog_reporter.report('Calculating IR intensities')

            self._compute_DOS(frequencies, ir_intensities, weights)
            mtd[self._out_ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._out_ws_name].setYUnitLabel('Intensity')

        # We want to create a DoS with Raman active
        elif self._spec_type == 'Raman_Active':
            if raman_intensities.size == 0:
                raise ValueError('Could not load any Raman intensities from file.')

            logger.notice('Calculating Raman intensities')
            prog_reporter.report('Calculating Raman intensities')

            self._compute_raman(frequencies, raman_intensities, weights)
            mtd[self._out_ws_name].setYUnit('A^4')
            mtd[self._out_ws_name].setYUnitLabel('Intensity')

        self.setProperty('OutputWorkspace', self._out_ws_name)

#----------------------------------------------------------------------------------------

    def _get_properties(self):
        """
        Set the properties passed to the algorithm
        """
        self._temperature = self.getProperty('Temperature').value
        self._bin_width = self.getProperty('BinWidth').value
        self._spec_type = self.getPropertyValue('SpectrumType')
        self._peak_func = self.getPropertyValue('Function')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')
        self._peak_width = self.getProperty('PeakWidth').value
        self._scale = self.getProperty('Scale').value
        self._zero_threshold = self.getProperty('ZeroThreshold').value
        self._ions = self.getProperty('Ions').value
        self._sum_contributions = self.getProperty('SumContributions').value
        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._calc_partial = (len(self._ions) > 0)

#----------------------------------------------------------------------------------------

    def _convert_to_cartesian_coordinates(self, unit_cell, ions):
        """
        Converts fractional coordinates to Cartesian coordinates given the unit
        cell vectors and adds to existing list of ions.

        @param unit_cell Unit cell vectors
        @param ions Ion list to be updated
        """
        for ion in ions:
            cell_pos = ion['fract_coord'] * unit_cell
            ion['cartesian_coord'] = np.apply_along_axis(np.sum, 0, cell_pos)

#----------------------------------------------------------------------------------------

    def _draw_peaks(self, xmin, hist, peaks):
        """
        Draw Gaussian or Lorentzian peaks to each point in the data

        @param xmin - minimum X value
        @param hist - array of counts for each bin
        @param peaks - the indicies of each non-zero point in the data
        @return the fitted y data
        """
        energies = np.arange(xmin, xmin + hist.size)

        if PEAK_WIDTH_ENERGY_FLAG in self._peak_width:
            try:
                peak_widths = np.fromiter([eval(self._peak_width.replace(PEAK_WIDTH_ENERGY_FLAG, str(energies[p])))\
                                           for p in peaks], dtype=float)
            except SyntaxError:
                raise ValueError('Invalid peak width function (must be either a decimal or function containing "energy")')
            peak_widths = np.abs(peak_widths)
            logger.debug('Peak widths: %s' % (str(peak_widths)))
        else:
            single_val = np.array([float(self._peak_width)])
            peak_widths = np.repeat(single_val, len(peaks))

        if self._peak_func == "Gaussian":
            n_gauss = int(3.0 * np.max(peak_widths))
            dos = np.zeros(len(hist) - 1 + n_gauss)

            for index, width in zip(peaks, peak_widths.tolist()):
                sigma = width / 2.354
                for g in range(-n_gauss, n_gauss):
                    if index + g > 0:
                        dos[index + g] += hist[index] * math.exp(-g ** 2 / (2 * sigma ** 2)) /\
                                          (math.sqrt(2 * math.pi) * sigma)

        elif self._peak_func == "Lorentzian":
            n_lorentz = int(25.0 * np.max(peak_widths))
            dos = np.zeros(len(hist) - 1 + n_lorentz)

            for index, width in zip(peaks, peak_widths.tolist()):
                gamma_by_2 = width / 2
                for l in range(-n_lorentz, n_lorentz):
                    if index + l > 0:
                        dos[index + l] += hist[index] * gamma_by_2 / (l ** 2 + gamma_by_2 ** 2) / math.pi

        return dos

#----------------------------------------------------------------------------------------

    def _draw_sticks(self, peaks, dos_shape):
        """
        Draw a stick diagram for peaks.

        @param hist - array of counts for each bin
        @param peaks - the indicies of each non-zero point in the data
        @param dos_shape - shape of the DOS array with broadened peaks
        @return the y data
        """
        dos = np.zeros(dos_shape)
        stick_intensity = self.getProperty('StickHeight').value

        for index in peaks:
            dos[index] = stick_intensity

        return dos

#----------------------------------------------------------------------------------------

    def _compute_partial_ion_workflow(self, partial_ions, frequencies, eigenvectors, weights):
        """
        Computes the partial DoS workspaces for a given set of ions (optionally scaling them by
        the cross scattering sections) and sums them into a single spectra.

        Both the partial workspaces and the summed total are returned.

        @param partial_ions Dict of ions to caculate DoS for
        @param frequencies Frequencies read from file
        @param eigenvectors Eigenvectors read from file
        @param weights Weights for each frequency block
        @returns Tuple of list of partial workspace names and summed contribution workspace name
        """
        logger.debug('Computing partial DoS for: ' + str(partial_ions))

        partial_workspaces = []
        total_workspace = None

        # Output each contribution to it's own workspace
        for ion_name, ions in partial_ions.items():
            partial_ws_name = self._out_ws_name + '_' + ion_name

            self._compute_partial(ions, frequencies, eigenvectors, weights)

            # Set correct units on partial workspace
            mtd[self._out_ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._out_ws_name].setYUnitLabel('Intensity')

            # Add the sample material to the workspace
            SetSampleMaterial(InputWorkspace=self._out_ws_name,
                              ChemicalFormula=ion_name)

            # Multiply intensity by scatttering cross section
            if self._scale_by_cross_section == 'Incoherent':
                scattering_x_section = mtd[self._out_ws_name].mutableSample().getMaterial().incohScatterXSection()
            elif self._scale_by_cross_section == 'Coherent':
                scattering_x_section = mtd[self._out_ws_name].mutableSample().getMaterial().cohScatterXSection()
            elif self._scale_by_cross_section == 'Total':
                scattering_x_section = mtd[self._out_ws_name].mutableSample().getMaterial().totalScatterXSection()

            if self._scale_by_cross_section != 'None':
                Scale(InputWorkspace=self._out_ws_name,
                      OutputWorkspace=self._out_ws_name,
                      Operation='Multiply',
                      Factor=scattering_x_section)

            partial_workspaces.append(partial_ws_name)
            RenameWorkspace(self._out_ws_name,
                            OutputWorkspace=partial_ws_name)

        total_workspace = self._out_ws_name + "_Total"

        # If there is more than one partial workspace need to sum first spectrum of all
        if len(partial_workspaces) > 1:
            data_x = mtd[partial_workspaces[0]].dataX(0)
            dos_specs = np.zeros_like(mtd[partial_workspaces[0]].dataY(0))
            stick_specs = np.zeros_like(mtd[partial_workspaces[0]].dataY(0))

            for partial_ws in partial_workspaces:
                dos_specs += mtd[partial_ws].dataY(0)
                stick_specs += mtd[partial_ws].dataY(1)

            stick_specs[stick_specs > 0.0] = self.getProperty('StickHeight').value

            self._create_dos_workspace(data_x, dos_specs, stick_specs, total_workspace)

            # Set correct units on total workspace
            mtd[total_workspace].setYUnit('(D/A)^2/amu')
            mtd[total_workspace].setYUnitLabel('Intensity')

        # Otherwise just repackage the WS we have as the total
        else:
            CloneWorkspace(InputWorkspace=partial_workspaces[0],
                           OutputWorkspace=total_workspace)

        logger.debug('Partial workspaces: ' + str(partial_workspaces))
        logger.debug('Summed workspace: ' + str(total_workspace))

        return partial_workspaces, total_workspace

#----------------------------------------------------------------------------------------

    def _compute_partial(self, ion_numbers, frequencies, eigenvectors, weights):
        """
        Compute partial Density Of States.

        This uses the eigenvectors in a .phonon file to calculate
        the partial density of states.

        @param ion_numbers - list of ion number to use in calculation
        @param frequencies - frequencies read from file
        @param eigenvectors - eigenvectors read from file
        @param weights - weights for each frequency block
        """

        intensities = []
        for block_vectors in eigenvectors:
            block_intensities = []
            for mode in xrange(self._num_branches):
                # Only select vectors for the ions we're interested in
                lower, upper = mode * self._num_ions, (mode + 1) * self._num_ions
                vectors = block_vectors[lower:upper]
                vectors = vectors[ion_numbers]

                # Compute intensity
                exponent = np.empty(vectors.shape)
                exponent.fill(2)
                vectors = np.power(vectors, exponent)
                total = np.sum(vectors)

                block_intensities.append(total)

            intensities += block_intensities

        intensities = np.asarray(intensities)
        self._compute_DOS(frequencies, intensities, weights)


#----------------------------------------------------------------------------------------

    def _compute_DOS(self, frequencies, intensities, weights):
        """
        Compute Density Of States

        @param frequencies - frequencies read from file
        @param intensities - intensities read from file
        @param weights - weights for each frequency block
        """
        if frequencies.size > intensities.size:
            # If we have less intensities than frequencies fill the difference with ones.
            diff = frequencies.size - intensities.size
            intensities = np.concatenate((intensities, np.ones(diff)))

        if frequencies.size != weights.size or frequencies.size != intensities.size:
            raise ValueError("Number of data points must match!")

        # Ignore values below fzerotol
        zero_mask = np.where(np.absolute(frequencies) < self._zero_threshold)
        intensities[zero_mask] = 0.0

        # Sort data to follow natural ordering
        permutation = frequencies.argsort()
        frequencies = frequencies[permutation]
        intensities = intensities[permutation]
        weights = weights[permutation]

        # Weight intensities
        intensities = intensities * weights

        # Create histogram x data
        xmin, xmax = frequencies[0], frequencies[-1] + 1
        bins = np.arange(xmin, xmax, 1)

        # Sum values in each bin
        hist = np.zeros(bins.size)
        for index, (lower, upper) in enumerate(zip(bins, bins[1:])):
            bin_mask = np.where((frequencies >= lower) & (frequencies < upper))
            hist[index] = intensities[bin_mask].sum()

        # Find and fit peaks
        peaks = hist.nonzero()[0]
        dos = self._draw_peaks(xmin, hist, peaks)
        dos_sticks = self._draw_sticks(peaks, dos.shape)

        data_x = np.arange(xmin, xmin + dos.size)
        self._create_dos_workspace(data_x, dos, dos_sticks, self._out_ws_name)

        if self._scale != 1:
            Scale(InputWorkspace=self._out_ws_name, OutputWorkspace=self._out_ws_name, Factor=self._scale)

        if self._bin_width != 1:
            out_ws = mtd[self._out_ws_name]
            x_min = out_ws.readX(0)[0] - (self._bin_width/2.0)
            x_max = out_ws.readX(0)[-1] + (self._bin_width/2.0)
            rebin_param = "%f, %f, %f" % (x_min, self._bin_width, x_max)
            Rebin(Inputworkspace=self._out_ws_name, Params=rebin_param, OutputWorkspace=self._out_ws_name)

#----------------------------------------------------------------------------------------

    def _create_dos_workspace(self, data_x, dos, dos_sticks, out_name):
        CreateWorkspace(DataX=data_x,
                        DataY=np.ravel(np.array([dos, dos_sticks])),
                        NSpec=2,
                        VerticalAxisUnit='Text',
                        VerticalAxisValues=[self._peak_func, 'Stick'],
                        OutputWorkspace=out_name,
                        EnableLogging=False)
        unitx = mtd[out_name].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Shift", 'cm^-1')

#----------------------------------------------------------------------------------------

    def _compute_raman(self, frequencies, intensities, weights):
        """
        Compute Raman intensities

        @param frequencies - frequencies read from file
        @param intensities - raman intensities read from file
        @param weights - weights for each frequency block
        """
        # We only want to use the first set
        frequencies = frequencies[:self._num_branches]
        intensities = intensities[:self._num_branches]
        weights = weights[:self._num_branches]

        # Speed of light in vaccum in m/s
        #c = scipy.constants.c #unused for now
        # Wavelength of the laser
        laser_wavelength = 514.5e-9
        # Planck's constant
        planck = scipy.constants.h
        # cm(-1) => K conversion
        cm1_to_K = scipy.constants.codata.value('inverse meter-kelvin relationship') * 100

        factor = (math.pow((2 * math.pi / laser_wavelength), 4) * planck) / (8 * math.pi ** 2 * 45) * 1e12
        x_sections = np.zeros(frequencies.size)

        # Use only the first set of frequencies and ignore small values
        zero_mask = np.where(frequencies > self._zero_threshold)
        frequency_x_sections = frequencies[zero_mask]
        intensity_x_sections = intensities[zero_mask]

        bose_occ = 1.0 / (np.exp(cm1_to_K * frequency_x_sections / self._temperature) - 1)
        x_sections[zero_mask] = factor / frequency_x_sections * (1 + bose_occ) * intensity_x_sections

        self._compute_DOS(frequencies, x_sections, weights)

#----------------------------------------------------------------------------------------

    def _read_data_from_file(self, file_name):
        """
        Select the appropriate file parser and check data was successfully
        loaded from file.

        @param file_name - path to the file.
        @return tuple of the frequencies, ir and raman intensities and weights
        """
        ext = os.path.splitext(file_name)[1]

        if ext == '.phonon':
            file_data = self._parse_phonon_file(file_name)
        elif ext == '.castep':
            if len(self._ions) > 0:
                raise ValueError("Cannot compute partial density of states from .castep files.")

            file_data = self._parse_castep_file(file_name)

        if file_data['frequencies'].size == 0:
            raise ValueError("Failed to load any frequencies from file.")

        return file_data

#----------------------------------------------------------------------------------------

    def _parse_block_header(self, header_match, block_count):
        """
        Parse the header of a block of frequencies and intensities

        @param header_match - the regex match to the header
        @param block_count - the count of blocks found so far
        @return weight for this block of values
        """
        # Found header block at start of frequencies
        q1, q2, q3, weight = [float(x) for x in header_match.groups()]
        q_vector = [q1, q2, q3]
        if block_count > 1 and sum(q_vector) == 0:
            weight = 0.0
        return weight, q_vector

#----------------------------------------------------------------------------------------

    def _parse_phonon_file_header(self, f_handle):
        """
        Read information from the header of a <>.phonon file

        @param f_handle - handle to the file.
        @return List of ions in file as list of tuple of (ion, mode number)
        """
        file_data = {'ions': []}

        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Number of ions' in line:
                self._num_ions = int(line.strip().split()[-1])
            elif 'Number of branches' in line:
                self._num_branches = int(line.strip().split()[-1])
            elif 'Unit cell vectors' in line:
                file_data['unit_cell'] = self._parse_phonon_unit_cell_vectors(f_handle)
            elif 'Fractional Co-ordinates' in line:
                if self._num_ions is None:
                    raise IOError("Failed to parse file. Invalid file header.")

                # Extract the mode number for each of the ion in the data file
                for _ in xrange(self._num_ions):
                    line = f_handle.readline()
                    line_data = line.strip().split()

                    species = line_data[4]
                    ion = {'species': species}
                    ion['fract_coord'] = np.array([float(line_data[1]), float(line_data[2]), float(line_data[3])])
                    # -1 to convert to zero based indexing
                    ion['index'] = int(line_data[0]) - 1
                    ion['bond_number'] = len([i for i in file_data['ions'] if i['species'] == species]) + 1
                    file_data['ions'].append(ion)

                logger.debug('All ions: ' + str(file_data['ions']))

            if 'END header' in line:
                if self._num_ions is None or self._num_branches is None:
                    raise IOError("Failed to parse file. Invalid file header.")
                return file_data

#----------------------------------------------------------------------------------------

    def _parse_phonon_freq_block(self, f_handle):
        """
        Iterator to parse a block of frequencies from a .phonon file.

        @param f_handle - handle to the file.
        """
        prog_reporter = Progress(self, 0.0, 1.0, 1)
        for _ in xrange(self._num_branches):
            line = f_handle.readline()
            line_data = line.strip().split()[1:]
            line_data = [float(x) for x in line_data]
            yield line_data

        prog_reporter.report("Reading frequencies.")

#----------------------------------------------------------------------------------------

    def _parse_phonon_unit_cell_vectors(self, f_handle):
        """
        Parses the unit cell vectors in a .phonon file.

        @param f_handle Handle to the file
        @return Numpy array of unit vectors
        """
        data = []
        for _ in range(3):
            line = f_handle.readline()
            line_data = line.strip().split()
            line_data = [float(x) for x in line_data]
            data.append(line_data)

        return np.array(data)

#----------------------------------------------------------------------------------------

    def _parse_phonon_eigenvectors(self, f_handle):
        vectors = []
        prog_reporter = Progress(self, 0.0, 1.0, self._num_branches * self._num_ions)
        for _ in xrange(self._num_ions * self._num_branches):
            line = f_handle.readline()

            if not line:
                raise IOError("Could not parse file. Invalid file format.")

            line_data = line.strip().split()
            vector_componets = line_data[2::2]
            vector_componets = [float(x) for x in vector_componets]
            vectors.append(vector_componets)
            prog_reporter.report("Reading eigenvectors.")

        return np.asarray(vectors)

#----------------------------------------------------------------------------------------

    def _parse_phonon_file(self, file_name):
        """
        Read frequencies from a <>.phonon file

        @param file_name - file path of the file to read
        @return the frequencies, infra red and raman intensities and weights of frequency blocks
        """
        file_data = {}

        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        header_regex_str = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': self._float_regex}
        header_regex = re.compile(header_regex_str)
        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        record_eigenvectors = self._calc_partial \
                           or (self._spec_type == 'DOS' and self._scale_by_cross_section != 'None') \
                           or self._spec_type == 'BondAnalysis'

        frequencies, ir_intensities, raman_intensities, weights, q_vectors, eigenvectors = [], [], [], [], [], []
        data_lists = (frequencies, ir_intensities, raman_intensities)
        with open(file_name, 'rU') as f_handle:
            file_data.update(self._parse_phonon_file_header(f_handle))

            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match = header_regex.match(line)
                if header_match:
                    block_count += 1

                    weight, q_vector = self._parse_block_header(header_match, block_count)
                    weights.append(weight)
                    q_vectors.append(q_vector)

                    # Parse block of frequencies
                    for line_data in self._parse_phonon_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

                vector_match = eigenvectors_regex.match(line)
                if vector_match:
                    if record_eigenvectors:
                        # Parse eigenvectors for partial dos
                        vectors = self._parse_phonon_eigenvectors(f_handle)
                        eigenvectors.append(vectors)
                    else:
                        # Skip over eigenvectors
                        for _ in xrange(self._num_ions * self._num_branches):
                            line = f_handle.readline()
                            if not line:
                                raise IOError("Bad file format. Uexpectedly reached end of file.")

        frequencies = np.asarray(frequencies)
        ir_intensities = np.asarray(ir_intensities)
        eigenvectors = np.asarray(eigenvectors)
        raman_intensities = np.asarray(raman_intensities)
        warray = np.repeat(weights, self._num_branches)

        file_data.update({
                'frequencies': frequencies,
                'ir_intensities': ir_intensities,
                'raman_intensities': raman_intensities,
                'weights': warray,
                'q_vectors':q_vectors,
                'eigenvectors': eigenvectors
                })

        return file_data

#----------------------------------------------------------------------------------------

    def _parse_castep_file_header(self, f_handle):
        """
        Read information from the header of a <>.castep file

        @param f_handle - handle to the file.
        @return tuple of the number of ions and branches in the file
        """
        num_species, self._num_ions = 0, 0
        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Total number of ions in cell =' in line:
                self._num_ions = int(line.strip().split()[-1])
            elif 'Total number of species in cell = ' in line:
                num_species = int(line.strip().split()[-1])

            if num_species > 0 and self._num_ions > 0:
                self._num_branches = num_species * self._num_ions
                return

#----------------------------------------------------------------------------------------

    def _parse_castep_freq_block(self, f_handle):
        """
        Iterator to parse a block of frequencies from a .castep file.

        @param f_handle - handle to the file.
        """
        prog_reporter = Progress(self, 0.0, 1.0, 1)
        for _ in xrange(self._num_branches):
            line = f_handle.readline()
            line_data = line.strip().split()[1:-1]
            freq = line_data[1]
            intensity_data = line_data[3:]

            # Remove non-active intensities from data
            intensities = []
            for value, active in zip(intensity_data[::2], intensity_data[1::2]):
                if self._spec_type == 'IR_Active' or self._spec_type == 'Raman_Active':
                    if active == 'N' and value != 0:
                        value = 0.0
                intensities.append(value)

            line_data = [freq] + intensities
            line_data = [float(x) for x in line_data]
            yield line_data

        prog_reporter.report("Reading frequencies.")

#----------------------------------------------------------------------------------------

    def _find_castep_freq_block(self, f_handle, data_regex):
        """
        Find the start of the frequency block in a .castep file.
        This will set the file pointer to the line before the start
        of the block.

        @param f_handle - handle to the file.
        """
        while True:
            pos = f_handle.tell()
            line = f_handle.readline()

            if not line:
                raise IOError("Could not parse frequency block. Invalid file format.")

            if data_regex.match(line):
                f_handle.seek(pos)
                return

#----------------------------------------------------------------------------------------

    def _parse_castep_bond(self, bond_match):
        """
        Parses a regex match to obtain bond information.

        @param bond_match Regex match to bond data line
        @return A dictionary defining the bond
        """
        bond = dict()

        bond['atom_a'] = (bond_match.group(1), int(bond_match.group(2)))
        bond['atom_b'] = (bond_match.group(3), int(bond_match.group(4)))
        bond['population'] = float(bond_match.group(5))
        bond['length'] = float(bond_match.group(6))

        return bond

#----------------------------------------------------------------------------------------

    def _parse_castep_file(self, file_name):
        """
        Read frequencies from a <>.castep file

        @param file_name - file path of the file to read
        @return the frequencies, infra red and raman intensities and weights of frequency blocks
        """
        # Header regex. Looks for lines in the following format:
        # +  q-pt=    1 (  0.000000  0.000000  0.000000)     1.0000000000              +
        header_regex_str = r" +\+ +q-pt= +\d+ \( *(?: *(%(s)s)) *(%(s)s) *(%(s)s)\) +(%(s)s) +\+" % {'s' : self._float_regex}
        header_regex = re.compile(header_regex_str)

        # Data regex. Looks for lines in the following format:
        #     +     1      -0.051481   a          0.0000000  N            0.0000000  N     +
        data_regex_str = r" +\+ +\d+ +(%(s)s)(?: +\w)? *(%(s)s)? *([YN])? *(%(s)s)? *([YN])? *\+" % {'s': self._float_regex}
        data_regex = re.compile(data_regex_str)

        # Atom bond regex. Looks for lines in the following format:
        #   H 006 --    O 012               0.46        1.04206
        bond_regex_str = r" +([A-z])+ +([0-9]+) +-- +([A-z]+) +([0-9]+) +(%(s)s) +(%(s)s)" % {'s': self._float_regex}
        bond_regex = re.compile(bond_regex_str)

        block_count = 0
        frequencies, ir_intensities, raman_intensities, weights, q_vectors, bonds = [], [], [], [], [], []
        data_lists = (frequencies, ir_intensities, raman_intensities)
        with open(file_name, 'rU') as f_handle:
            self._parse_castep_file_header(f_handle)

            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match = header_regex.match(line)
                if header_match:
                    block_count += 1
                    weight, q_vector = self._parse_block_header(header_match, block_count)
                    weights.append(weight)
                    q_vectors.append(q_vector)

                    # Move file pointer forward to start of intensity data
                    self._find_castep_freq_block(f_handle, data_regex)

                    # Parse block of frequencies
                    for line_data in self._parse_castep_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

                # Check if we've found a bond
                bond_match = bond_regex.match(line)
                if bond_match:
                    bonds.append(self._parse_castep_bond(bond_match))

        frequencies = np.asarray(frequencies)
        ir_intensities = np.asarray(ir_intensities)
        raman_intensities = np.asarray(raman_intensities)
        warray = np.repeat(weights, self._num_branches)

        file_data = {
                'frequencies': frequencies,
                'ir_intensities': ir_intensities,
                'raman_intensities': raman_intensities,
                'weights': warray,
                'q_vectors':q_vectors
                }

        if len(bonds) > 0:
            file_data['bonds'] = bonds

        return file_data

#----------------------------------------------------------------------------------------

try:
    import scipy.constants
    AlgorithmFactory.subscribe(SimulatedDensityOfStates)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package scipy may be missing.')
