from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np
import re
import os.path
import math

class DensityOfStates(PythonAlgorithm):

    _float_regex = None
    _temperature = None
    _bin_width = None
    _spec_type = None
    _peak_func = None
    _ws_name = None
    _peak_width = None
    _scale = None
    _zero_threshold = None
    _ions = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _calc_partial = None
    _ion_dict = None
    _partial_ion_numbers = None
    _num_ions = None
    _num_branches = None

    def summary(self):
        return "Calculates phonon densities of states, Raman and IR spectrum."

    def PyInit(self):
        # Declare properties
        self.declareProperty(FileProperty('File', '', action=FileAction.Load,
            extensions = ["phonon", "castep"]),
            doc='Filename of the file.')

        self.declareProperty(name='Function',defaultValue='Gaussian',
            validator=StringListValidator(['Gaussian', 'Lorentzian']),
            doc="Type of function to fit to peaks.")

        self.declareProperty(name='PeakWidth', defaultValue=10.0,
            doc='Set Gaussian/Lorentzian FWHM for broadening. Default is 10')

        self.declareProperty(name='SpectrumType',defaultValue='DOS',
            validator=StringListValidator(['IonTable', 'DOS', 'IR_Active', 'Raman_Active']),
            doc="Type of intensities to extract and model (fundamentals-only) from .phonon.")

        self.declareProperty(name='Scale', defaultValue=1.0,
            doc='Scale the intesity by the given factor. Default is no scaling.')

        self.declareProperty(name='BinWidth', defaultValue=1.0,
            doc='Set histogram resolution for binning (eV or cm**-1). Default is 1')

        self.declareProperty(name='Temperature', defaultValue=300.0,
            doc='Temperature to use (in raman spectrum modelling). Default is 300')

        self.declareProperty(name='ZeroThreshold', defaultValue=3.0,
            doc='Ignore frequencies below the this threshold. Default is 3.0')

        self.declareProperty(StringArrayProperty('Ions', Direction.Input),
            doc="List of Ions to use to calculate partial density of states. If left blank, total density of states will be calculated")

        self.declareProperty(name='SumContributions', defaultValue=False,
            doc="Sum the partial density of states into a single workspace.")

        self.declareProperty(name='ScaleByCrossSection', defaultValue='None',
            validator=StringListValidator(['None', 'Total', 'Incoherent', 'Coherent']),
            doc="Sum the partial density of states by the scattering cross section.")

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
            doc="Name to give the output workspace.")

        # Regex pattern for a floating point number
        self._float_regex = '\-?(?:\d+\.?\d*|\d*\.?\d+)'

#----------------------------------------------------------------------------------------

    def validateInputs(self):
        """
        Performs input validation.

        Used to ensure the user is requesting a valid mode.
        """
        issues = dict()

        file_name = self.getPropertyValue('File')
        file_type = file_name[file_name.rfind('.') + 1:]
        spec_type = self.getPropertyValue('SpectrumType')
        sum_contributions = self.getProperty('SumContributions').value
        scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection') != 'None'

        ions = self.getProperty('Ions').value
        calc_partial = len(ions) > 0

        if spec_type == 'IonTable' and file_type != 'phonon':
            issues['SpectrumType'] = 'Cannot output an ion table from a %s file' % file_type

        if spec_type != 'DOS' and calc_partial:
            issues['Ions'] = 'Cannot calculate partial density of states when using %s' % spec_type

        if spec_type != 'DOS' and scale_by_cross_section:
            issues['ScaleByCrossSection'] = 'Cannot scale contributions by cross sections when using %s' % spec_type

        if not calc_partial and sum_contributions:
            issues['SumContributions'] = 'Cannot sum contributions when not calculating partial density of states'

        return issues

#----------------------------------------------------------------------------------------

    def PyExec(self):
        # Run the algorithm
        self._get_properties()

        file_name = self.getPropertyValue('File')
        file_data = self._read_data_from_file(file_name)
        frequencies, ir_intensities, raman_intensities, weights = file_data[:4]

        prog_reporter = Progress(self, 0.0, 1.0, 1)

        # We want to output a table workspace with ion information
        if self._spec_type == 'IonTable':
            ion_table = CreateEmptyTableWorkspace(OutputWorkspace=self._ws_name)
            ion_table.addColumn('str', 'Ion')
            ion_table.addColumn('int', 'Count')

            for ion, data in self._ion_dict.items():
                ion_table.addRow([ion, len(data)])

        # We want to calculate a partial DoS
        elif self._calc_partial and self._spec_type == 'DOS':
            logger.notice('Calculating partial density of states')
            prog_reporter.report('Calculating partial density of states')

            eigenvectors = file_data[4]

            # Filter the dict of all ions to only those the user cares about
            partial_ions = dict()
            for k, v in self._ion_dict.items():
                if k in self._ions:
                    partial_ions[k] = v

            partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(
                                                    partial_ions, frequencies,
                                                    eigenvectors, weights)

            if self._sum_contributions:
                # Discard the partial workspaces
                for partial_ws in partial_workspaces:
                    DeleteWorkspace(partial_ws)

                # Rename the summed workspace, this will be the output
                RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._ws_name)

            else:
                DeleteWorkspace(sum_workspace)

                group = ','.join(partial_workspaces)
                GroupWorkspaces(group, OutputWorkspace=self._ws_name)

        # We want to calculate a total DoS with scaled intensities
        elif self._spec_type == 'DOS' and self._scale_by_cross_section != 'None':
            logger.notice('Calculating summed density of states with scaled intensities')
            prog_reporter.report('Calculating density of states')

            eigenvectors = file_data[4]

            partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(
                                                    self._ion_dict, frequencies,
                                                    eigenvectors, weights)

            # Discard the partial workspaces
            for partial_ws in partial_workspaces:
                DeleteWorkspace(partial_ws)

            # Rename the summed workspace, this will be the output
            RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._ws_name)

        # We want to calculate a total DoS without scaled intensities
        elif self._spec_type == 'DOS':
            logger.notice('Calculating summed density of states without scaled intensities')
            prog_reporter.report('Calculating density of states')

            self._compute_DOS(frequencies, np.ones_like(frequencies), weights)
            mtd[self._ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._ws_name].setYUnitLabel('Intensity')

        # We want to calculate a DoS with IR active
        elif self._spec_type == 'IR_Active':
            if ir_intensities.size == 0:
                raise ValueError('Could not load any IR intensities from file.')

            logger.notice('Calculating IR intensities')
            prog_reporter.report('Calculating IR intensities')

            self._compute_DOS(frequencies, ir_intensities, weights)
            mtd[self._ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._ws_name].setYUnitLabel('Intensity')

        # We want to create a DoS with Raman active
        elif self._spec_type == 'Raman_Active':
            if raman_intensities.size == 0:
                raise ValueError('Could not load any Raman intensities from file.')

            logger.notice('Calculating Raman intensities')
            prog_reporter.report('Calculating Raman intensities')

            self._compute_raman(frequencies, raman_intensities, weights)
            mtd[self._ws_name].setYUnit('A^4')
            mtd[self._ws_name].setYUnitLabel('Intensity')

        self.setProperty('OutputWorkspace', self._ws_name)

#----------------------------------------------------------------------------------------

    def _get_properties(self):
        """
        Set the properties passed to the algorithm
        """
        self._temperature = self.getProperty('Temperature').value
        self._bin_width = self.getProperty('BinWidth').value
        self._spec_type = self.getPropertyValue('SpectrumType')
        self._peak_func = self.getPropertyValue('Function')
        self._ws_name = self.getPropertyValue('OutputWorkspace')
        self._peak_width = self.getProperty('PeakWidth').value
        self._scale = self.getProperty('Scale').value
        self._zero_threshold = self.getProperty('ZeroThreshold').value
        self._ions = self.getProperty('Ions').value
        self._sum_contributions = self.getProperty('SumContributions').value
        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._calc_partial = (len(self._ions) > 0)

#----------------------------------------------------------------------------------------

    def _draw_peaks(self, hist, peaks):
        """
        Draw Gaussian or Lorentzian peaks to each point in the data

        @param hist - array of counts for each bin
        @param peaks - the indicies of each non-zero point in the data
        @return the fitted y data
        """
        if self._peak_func == "Gaussian":
            n_gauss = int(3.0 * self._peak_width / self._bin_width)
            sigma = self._peak_width / 2.354

            dos = np.zeros(len(hist) - 1 + n_gauss)

            for index in peaks:
                for g in range(-n_gauss, n_gauss):
                    if index + g > 0:
                        dos[index + g] += hist[index] * math.exp(-(g * self._bin_width) ** 2 / (2 * sigma ** 2)) / (math.sqrt(2 * math.pi) * sigma)

        elif self._peak_func == "Lorentzian":

            n_lorentz = int(25.0 * self._peak_width / self._bin_width)
            gamma_by_2 = self._peak_width / 2

            dos = np.zeros(len(hist) - 1 + n_lorentz)

            for index in peaks:
                for l in range(-n_lorentz, n_lorentz):
                    if index + l > 0:
                        dos[index + l] += hist[index] * gamma_by_2 / (l ** 2 + gamma_by_2 ** 2) / math.pi

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
            partial_ws_name = self._ws_name + '_' + ion_name

            self._compute_partial(ions, frequencies, eigenvectors, weights)

            # Set correct units on partial workspace
            mtd[self._ws_name].setYUnit('(D/A)^2/amu')
            mtd[self._ws_name].setYUnitLabel('Intensity')

            # Add the sample material to the workspace
            SetSampleMaterial(InputWorkspace=self._ws_name, ChemicalFormula=ion_name)

            # Multiply intensity by scatttering cross section
            if self._scale_by_cross_section == 'Incoherent':
                scattering_x_section = mtd[self._ws_name].mutableSample().getMaterial().incohScatterXSection()
            elif self._scale_by_cross_section == 'Coherent':
                scattering_x_section = mtd[self._ws_name].mutableSample().getMaterial().cohScatterXSection()
            elif self._scale_by_cross_section == 'Total':
                scattering_x_section = mtd[self._ws_name].mutableSample().getMaterial().totalScatterXSection()

            if self._scale_by_cross_section != 'None':
                Scale(InputWorkspace=self._ws_name, OutputWorkspace=self._ws_name,
                      Operation='Multiply', Factor=scattering_x_section)

            partial_workspaces.append(partial_ws_name)
            RenameWorkspace(self._ws_name, OutputWorkspace=partial_ws_name)

        total_workspace = self._ws_name + "_Total"

        # If there is more than one partial workspace need to sum first spectrum of all
        if len(partial_workspaces) > 1:
            sum_workspace = '__dos_sum'

            # Collect spectra into a single workspace
            AppendSpectra(OutputWorkspace=sum_workspace, InputWorkspace1=partial_workspaces[0],
                          InputWorkspace2=partial_workspaces[1])
            for ws_idx in xrange(2, len(partial_workspaces)):
                AppendSpectra(OutputWorkspace=sum_workspace, InputWorkspace1=sum_workspace,
                              InputWorkspace2=partial_workspaces[ws_idx])

            # Sum all spectra
            SumSpectra(InputWorkspace=sum_workspace, OutputWorkspace=total_workspace)
            mtd[total_workspace].getSpectrum(0).setSpectrumNo(1)

            # Remove workspace used to sum spectra
            DeleteWorkspace(sum_workspace)

        # Otherwise just repackage the WS we have as the total
        else:
            CloneWorkspace(InputWorkspace=partial_workspaces[0], OutputWorkspace=total_workspace)

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
        xmin, xmax = frequencies[0], frequencies[-1] + self._bin_width
        bins = np.arange(xmin, xmax, self._bin_width)

        # Sum values in each bin
        hist = np.zeros(bins.size)
        for index, (lower, upper) in enumerate(zip(bins, bins[1:])):
            bin_mask = np.where((frequencies >= lower) & (frequencies < upper))
            hist[index] = intensities[bin_mask].sum()

        # Find and fit peaks
        peaks = hist.nonzero()[0]
        dos = self._draw_peaks(hist, peaks)

        data_x = np.arange(xmin, xmin + dos.size)
        CreateWorkspace(DataX=data_x, DataY=dos, OutputWorkspace=self._ws_name)
        unitx = mtd[self._ws_name].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Shift", 'cm^-1')

        if self._scale != 1:
            Scale(InputWorkspace=self._ws_name, OutputWorkspace=self._ws_name, Factor=self._scale)

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
        c = scipy.constants.c
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

        frequencies = file_data[0]

        if frequencies.size == 0:
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
        q1, q2, q3, weight = map(float, header_match.groups())
        if block_count > 1 and sum([q1, q2, q3]) == 0:
            weight = 0.0
        return weight

#----------------------------------------------------------------------------------------

    def _parse_phonon_file_header(self, f_handle):
        """
        Read information from the header of a <>.phonon file

        @param f_handle - handle to the file.
        @return tuple of the number of ions and branches in the file
        """
        while True:
            line = f_handle.readline()

            if not line:
                raise IOError("Could not find any header information.")

            if 'Number of ions' in line:
                self._num_ions = int(line.strip().split()[-1])
            elif 'Number of branches' in line:
                self._num_branches = int(line.strip().split()[-1])
            elif 'Fractional Co-ordinates' in line:
                self._ion_dict = dict()

                if self._num_ions is None:
                    raise IOError("Failed to parse file. Invalid file header.")

                # Extract the mode number for each of the ion in the data file
                for _ in xrange(self._num_ions):
                    line = f_handle.readline()
                    line_data = line.strip().split()
                    ion = line_data[4]

                    mode = int(line_data[0]) - 1  # -1 to convert to zero based indexing

                    # Add the ion and the mode to the dict
                    if ion not in self._ion_dict:
                        self._ion_dict[ion] = list()
                    self._ion_dict[ion].append(mode)

                logger.debug('All ions: ' + str(self._ion_dict))

                self._partial_ion_numbers = []
                for ion, ion_nums in self._ion_dict.items():
                    if len(ion_nums) == 0:
                        logger.warning("Could not find any ions of type %s" % ion)
                    self._partial_ion_numbers += ion_nums

                self._partial_ion_numbers = sorted(self._partial_ion_numbers)
                self._partial_ion_numbers = np.asarray(self._partial_ion_numbers)

                if self._partial_ion_numbers.size == 0:
                    raise ValueError("Could not find any of the specified ions")

            if 'END header' in line:
                if self._num_ions is None or self._num_branches is None:
                    raise IOError("Failed to parse file. Invalid file header.")
                return

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
            line_data = map(float, line_data)
            yield line_data

        prog_reporter.report("Reading frequencies.")

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
            vector_componets = map(float, vector_componets)
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
        # Header regex. Looks for lines in the following format:
        #     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
        header_regex_str = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': self._float_regex}
        header_regex = re.compile(header_regex_str)

        eigenvectors_regex = re.compile(r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*")
        block_count = 0

        frequencies, ir_intensities, raman_intensities, weights = [], [], [], []
        eigenvectors = []
        data_lists = (frequencies, ir_intensities, raman_intensities)
        with open(file_name, 'rU') as f_handle:
            self._parse_phonon_file_header(f_handle)

            while True:
                line = f_handle.readline()
                # Check we've reached the end of file
                if not line:
                    break

                # Check if we've found a block of frequencies
                header_match = header_regex.match(line)
                if header_match:
                    block_count += 1

                    weight = self._parse_block_header(header_match, block_count)
                    weights.append(weight)

                    # Parse block of frequencies
                    for line_data in self._parse_phonon_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

                vector_match = eigenvectors_regex.match(line)
                if vector_match:
                    if self._calc_partial or (self._spec_type == 'DOS' and self._scale_by_cross_section != 'None'):
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

        return frequencies, ir_intensities, raman_intensities, warray, eigenvectors

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
            line_data = map(float, line_data)
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
        data_regex_str = r" +\+ +\d+ +(%(s)s)(?: +\w)? *(%(s)s)? *([YN])? *(%(s)s)? *([YN])? *\+"% {'s': self._float_regex}
        data_regex = re.compile(data_regex_str)

        block_count = 0
        frequencies, ir_intensities, raman_intensities, weights = [], [], [], []
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
                    weight = self._parse_block_header(header_match, block_count)
                    weights.append(weight)

                    # Move file pointer forward to start of intensity data
                    self._find_castep_freq_block(f_handle, data_regex)

                    # Parse block of frequencies
                    for line_data in self._parse_castep_freq_block(f_handle):
                        for data_list, item in zip(data_lists, line_data):
                            data_list.append(item)

        frequencies = np.asarray(frequencies)
        ir_intensities = np.asarray(ir_intensities)
        raman_intensities = np.asarray(raman_intensities)
        warray = np.repeat(weights, self._num_branches)

        return frequencies, ir_intensities, raman_intensities, warray


try:
    import scipy.constants
    AlgorithmFactory.subscribe(DensityOfStates)
except:
    logger.debug('Failed to subscribe algorithm DensityOfStates; The python package scipy may be missing.')
