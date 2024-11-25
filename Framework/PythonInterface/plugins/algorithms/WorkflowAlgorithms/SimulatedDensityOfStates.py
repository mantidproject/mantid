# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-many-lines, redefined-builtin
import numpy as np
import re
import os.path
import math
from collections import OrderedDict

import scipy.constants

from mantid.api import AlgorithmFactory, FileAction, FileProperty, Progress, PythonAlgorithm, WorkspaceProperty
from mantid.kernel import logger, Direction, StringArrayProperty, StringListValidator
import mantid.simpleapi as s_api


PEAK_WIDTH_ENERGY_FLAG = "energy"


class SimulatedDensityOfStates(PythonAlgorithm):
    _spec_type = None
    _peak_func = None
    _out_ws_name = None
    _peak_width = None
    _zero_threshold = None
    _ions_of_interest = None
    _scale_by_cross_section = None
    _calc_partial = None
    _num_ions = None
    _num_branches = None
    _element_isotope = dict()

    def category(self):
        return "Simulation"

    def summary(self):
        return "Calculates phonon densities of states, Raman and IR spectrum."

    def PyInit(self):
        # Declare properties
        self.declareProperty(
            FileProperty("CASTEPFile", "", action=FileAction.OptionalLoad, extensions=["castep"]), doc="Filename of the CASTEP file."
        )

        self.declareProperty(
            FileProperty("PHONONFile", "", action=FileAction.OptionalLoad, extensions=["phonon"]), doc="Filename of the PHONON file."
        )

        self.declareProperty(
            FileProperty("ForceConstantsFile", "", action=FileAction.OptionalLoad, extensions=[".castep_bin", ".json", ".yaml"])
        )

        self.declareProperty(
            name="ForceConstantsSampling", defaultValue=20.0, doc="Real-space cutoff in Angstrom for Brillouin zone sampling."
        )

        self.declareProperty(
            name="Function",
            defaultValue="Gaussian",
            validator=StringListValidator(["Gaussian", "Lorentzian"]),
            doc="Type of function to fit to peaks.",
        )

        self.declareProperty(name="PeakWidth", defaultValue="10.0", doc="Set Gaussian/Lorentzian FWHM for broadening. Default is 10")

        self.declareProperty(
            name="SpectrumType",
            defaultValue="DOS",
            validator=StringListValidator(["IonTable", "DOS", "IR_Active", "Raman_Active", "BondTable"]),
            doc="Type of intensities to extract and model (fundamentals-only) from .phonon.",
        )

        self.declareProperty(
            name="CalculateIonIndices", defaultValue=False, doc="Calculates the individual index of all Ions in the simulated data."
        )

        self.declareProperty(name="StickHeight", defaultValue=0.01, doc="Intensity of peaks in stick diagram.")

        self.declareProperty(name="Scale", defaultValue=1.0, doc="Scale the intesity by the given factor. Default is no scaling.")

        self.declareProperty(name="BinWidth", defaultValue=1.0, doc="Set histogram resolution for binning (eV or cm**-1). Default is 1")

        self.declareProperty(name="Temperature", defaultValue=300.0, doc="Temperature to use (in raman spectrum modelling). Default is 300")

        self.declareProperty(name="ZeroThreshold", defaultValue=3.0, doc="Ignore frequencies below the this threshold. Default is 3.0")

        self.declareProperty(
            StringArrayProperty("Ions", Direction.Input),
            doc="List of Ions to use to calculate partial density of states." "If left blank, total density of states will be calculated",
        )

        self.declareProperty(name="SumContributions", defaultValue=False, doc="Sum the partial density of states into a single workspace.")

        self.declareProperty(
            name="ScaleByCrossSection",
            defaultValue="None",
            validator=StringListValidator(["None", "Total", "Incoherent", "Coherent"]),
            doc="Sum the partial density of states by the scattering cross section.",
        )

        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", Direction.Output), doc="Name to give the output workspace.")

    def validateInputs(self):
        """
        Performs input validation.

        Used to ensure the user is requesting a valid mode.
        """
        issues = dict()

        castep_filename = self.getPropertyValue("CASTEPFile")
        phonon_filename = self.getPropertyValue("PHONONFile")
        euphonic_filename = self.getPropertyValue("ForceConstantsFile")

        pdos_available = bool(phonon_filename or euphonic_filename)

        if not any((castep_filename, phonon_filename, euphonic_filename)):
            msg = "Must have at least one input file"
            issues["CASTEPFile"] = msg
            issues["PHONONFile"] = msg
            issues["ForceConstantsFile"] = msg

        spec_type = self.getPropertyValue("SpectrumType")
        sum_contributions = self.getProperty("SumContributions").value
        scale_by_cross_section = self.getPropertyValue("ScaleByCrossSection") != "None"

        ions = self.getProperty("Ions").value
        calc_partial = len(ions) > 0

        if spec_type == "IonTable" and not pdos_available:
            issues["SpectrumType"] = "Cannot produce ion table when only .castep file is provided"

        if spec_type == "BondAnalysis" and phonon_filename == "" and castep_filename == "":
            issues["SpectrumType"] = "Require both a .phonon and .castep file for bond analysis"

        if spec_type == "BondTable" and castep_filename == "":
            issues["SpectrumType"] = "Require a .castep file for bond table output"

        if spec_type != "DOS" and calc_partial:
            issues["Ions"] = "Cannot calculate partial density of states when using %s" % spec_type

        if spec_type != "DOS" and scale_by_cross_section:
            issues["ScaleByCrossSection"] = "Cannot scale contributions by cross sections when using %s" % spec_type

        if scale_by_cross_section and not pdos_available:
            issues["ScaleByCrossSection"] = "Cannot scale by cross sections when only .castep file is provided"

        if not calc_partial and sum_contributions:
            issues["SumContributions"] = "Cannot sum contributions when not calculating partial density of states"

        return issues

    def PyExec(self):
        # Run the algorithm
        self._get_properties()

        file_data = self._read_file()

        # Get variables from file_data
        frequencies = file_data["frequencies"]
        ir_intensities = file_data["ir_intensities"]
        raman_intensities = file_data["raman_intensities"]
        weights = file_data["weights"]
        eigenvectors = file_data.get("eigenvectors", None)
        ion_data = file_data.get("ions", None)
        unit_cell = file_data.get("unit_cell", None)
        self._num_branches = file_data["num_branches"]

        logger.debug("Unit cell: {0}".format(unit_cell))

        prog_reporter = Progress(self, 0.0, 1.0, 1)

        # Output a table workspace with ion information
        if self._spec_type == "IonTable":
            self._create_ion_table(unit_cell, ion_data)

        # Output a table workspace with bond information
        if self._spec_type == "BondTable":
            bonds = file_data.get("bonds", None)
            self._create_bond_table(bonds)

        # Calculate a partial DoS
        elif self._calc_partial and self._spec_type == "DOS":
            logger.notice("Calculating partial density of states")
            prog_reporter.report("Calculating partial density of states")
            self._calculate_partial_dos(ion_data, frequencies, eigenvectors, weights)

        # Calculate a total DoS with scaled intensities
        elif self._spec_type == "DOS" and self._scale_by_cross_section != "None":
            logger.notice("Calculating summed density of states with scaled intensities")
            prog_reporter.report("Calculating density of states")
            self._calculate_total_dos_with_scale(ion_data, frequencies, eigenvectors, weights)

        # Calculate a total DoS without scaled intensities
        elif self._spec_type == "DOS":
            logger.notice("Calculating summed density of states without scaled intensities")
            prog_reporter.report("Calculating density of states")

            out_ws = self._compute_DOS(frequencies, np.ones_like(frequencies), weights)
            out_ws.setYUnit("(D/A)^2/amu")
            out_ws.setYUnitLabel("Intensity")

        # Calculate a DoS with IR active
        elif self._spec_type == "IR_Active":
            if ir_intensities.size == 0:
                raise ValueError("Could not load any IR intensities from file.")

            logger.notice("Calculating IR intensities")
            prog_reporter.report("Calculating IR intensities")

            out_ws = self._compute_DOS(frequencies, ir_intensities, weights)
            out_ws.setYUnit("(D/A)^2/amu")
            out_ws.setYUnitLabel("Intensity")

        # Create a DoS with Raman active
        elif self._spec_type == "Raman_Active":
            if raman_intensities.size == 0:
                raise ValueError("Could not load any Raman intensities from file.")

            logger.notice("Calculating Raman intensities")
            prog_reporter.report("Calculating Raman intensities")

            out_ws = self._compute_raman(frequencies, raman_intensities, weights)
            out_ws.setYUnit("A^4")
            out_ws.setYUnitLabel("Intensity")

        self.setProperty("OutputWorkspace", self._out_ws_name)

    def _get_properties(self):
        """
        Set the properties passed to the algorithm
        """
        self._spec_type = self.getPropertyValue("SpectrumType")
        self._peak_func = self.getPropertyValue("Function")
        self._out_ws_name = self.getPropertyValue("OutputWorkspace")
        self._peak_width = self.getProperty("PeakWidth").value
        self._zero_threshold = self.getProperty("ZeroThreshold").value
        self._ions_of_interest = self.getProperty("Ions").value
        self._scale_by_cross_section = self.getPropertyValue("ScaleByCrossSection")
        self._calc_partial = len(self._ions_of_interest) > 0

    def _read_file(self):
        """
        Decides if a castep or phonon file should be read then reads the file data
        Raises RuntimeError if no valid file is found.

        @return file_data dictionary holding all required data from the castep or phonon file
        """
        from dos.load_euphonic import get_data_with_euphonic

        castep_filename = self.getPropertyValue("CASTEPFile")
        phonon_filename = self.getPropertyValue("PHONONFile")
        euphonic_filename = self.getPropertyValue("ForceConstantsFile")

        if phonon_filename and self._spec_type != "BondTable":
            return self._read_data_from_file(phonon_filename)
        elif castep_filename:
            return self._read_data_from_file(castep_filename)
        elif euphonic_filename:
            file_data, self._element_isotope = get_data_with_euphonic(
                euphonic_filename, cutoff=float(self.getPropertyValue("ForceConstantsSampling")), acoustic_sum_rule=None
            )
            self._num_ions = file_data["num_ions"]
            return file_data

        else:
            raise RuntimeError("No valid data file")

    def _create_ion_table(self, unit_cell, ions):
        """
        Creates an ion table from the ions and unit cell in the file_data object
        populated when the phonon/castep file is parsed.
        @param unit_cell    :: The unit cell read from the castep/phonon file
        @param ions         :: The ion data obtained from the castep/phonon file
        """
        ion_table = s_api.CreateEmptyTableWorkspace(OutputWorkspace=self._out_ws_name)
        ion_table.addColumn("str", "Species")
        ion_table.addColumn("int", "FileIndex")
        ion_table.addColumn("int", "Number")
        ion_table.addColumn("float", "FractionalX")
        ion_table.addColumn("float", "FractionalY")
        ion_table.addColumn("float", "FractionalZ")
        ion_table.addColumn("float", "CartesianX")
        ion_table.addColumn("float", "CartesianY")
        ion_table.addColumn("float", "CartesianZ")
        ion_table.addColumn("float", "Isotope")

        self._convert_to_cartesian_coordinates(unit_cell, ions)

        for ion in ions:
            ion_table.addRow(
                [
                    ion["species"],
                    ion["index"],
                    ion["bond_number"],
                    ion["fract_coord"][0],
                    ion["fract_coord"][1],
                    ion["fract_coord"][2],
                    ion["cartesian_coord"][0],
                    ion["cartesian_coord"][1],
                    ion["cartesian_coord"][2],
                    ion["isotope_number"],
                ]
            )

    def _create_bond_table(self, bonds):
        """
        Creates a bond table from the bond data obtained when the castep file is read
        @param bonds       :: The bond data read from the castep file
        """
        if bonds is None or len(bonds) == 0:
            raise RuntimeError("No bonds found in CASTEP file")

        bond_table = s_api.CreateEmptyTableWorkspace(OutputWorkspace=self._out_ws_name)
        bond_table.addColumn("str", "SpeciesA")
        bond_table.addColumn("int", "NumberA")
        bond_table.addColumn("str", "SpeciesB")
        bond_table.addColumn("int", "NumberB")
        bond_table.addColumn("float", "Length")
        bond_table.addColumn("float", "Population")

        for bond in bonds:
            bond_table.addRow(
                [bond["atom_a"][0], bond["atom_a"][1], bond["atom_b"][0], bond["atom_b"][1], bond["length"], bond["population"]]
            )

    def _calculate_partial_dos(self, ions, frequencies, eigenvectors, weights):
        """
        Calculate the partial Density of States for all the ions of interest to the user
        @param frequencies      :: frequency data from file
        @param eigenvectors     :: eigenvector data from file
        @param weights          :: weight data from file
        """
        # Build a dictionary of ions that the user cares about
        # systemtests check order so use OrderedDict
        partial_ions = OrderedDict()

        calc_ion_index = self.getProperty("CalculateIonIndices").value

        if not calc_ion_index:
            for ion in self._ions_of_interest:
                partial_ions[ion] = [i["index"] for i in ions if i["species"] == ion]
        else:
            for ion in ions:
                if ion["species"] in self._ions_of_interest:
                    ion_identifier = ion["species"] + str(ion["index"])
                    partial_ions[ion_identifier] = ion["index"]

        partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(partial_ions, frequencies, eigenvectors, weights)

        if self.getProperty("SumContributions").value:
            # Discard the partial workspaces
            for partial_ws in partial_workspaces:
                s_api.DeleteWorkspace(partial_ws)

            # Rename the summed workspace, this will be the output
            s_api.RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._out_ws_name)

        else:
            s_api.DeleteWorkspace(sum_workspace)
            partial_ws_names = [ws.name() for ws in partial_workspaces]
            # Sort workspaces
            if calc_ion_index:
                # Sort by index after '_'
                partial_ws_names.sort(key=lambda item: (int(item[(item.rfind("_") + 1) :])))
            group = ",".join(partial_ws_names)
            s_api.GroupWorkspaces(group, OutputWorkspace=self._out_ws_name)

    def _calculate_total_dos_with_scale(self, ions, frequencies, eigenvectors, weights):
        """
        Calculate the complete Density of States for all the ions of interest to the user with scaled intensities
        @param frequencies      :: frequency data from file
        @param eigenvectors     :: eigenvector data from file
        @param weights          :: weight data from file
        """
        # Build a dict of all ions
        all_ions = dict()
        for ion in set([i["species"] for i in ions]):
            all_ions[ion] = [i["index"] for i in ions if i["species"] == ion]

        partial_workspaces, sum_workspace = self._compute_partial_ion_workflow(all_ions, frequencies, eigenvectors, weights)

        # Discard the partial workspaces
        for partial_ws in partial_workspaces:
            s_api.DeleteWorkspace(partial_ws)

        # Rename the summed workspace, this will be the output
        s_api.RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._out_ws_name)

    def _convert_to_cartesian_coordinates(self, unit_cell, ions):
        """
        Converts fractional coordinates to Cartesian coordinates given the unit
        cell vectors and adds to existing list of ions.

        @param unit_cell Unit cell vectors
        @param ions Ion list to be updated
        """
        for ion in ions:
            cell_pos = ion["fract_coord"] * unit_cell
            ion["cartesian_coord"] = np.apply_along_axis(np.sum, 0, cell_pos)

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
                peak_widths = np.fromiter(
                    [eval(self._peak_width.replace(PEAK_WIDTH_ENERGY_FLAG, str(energies[p]))) for p in peaks], dtype=float
                )
            except SyntaxError:
                raise ValueError('Invalid peak width function (must be either a decimal or function containing "energy")')
            peak_widths = np.abs(peak_widths)
            logger.debug("Peak widths: %s" % (str(peak_widths)))
        else:
            try:
                single_val = np.array([float(self._peak_width)])
            except ValueError:
                raise ValueError('Invalid peak width function (must be either a decimal or function containing "energy")')
            peak_widths = np.repeat(single_val, len(peaks))

        if self._peak_func == "Gaussian":
            n_gauss = int(3.0 * np.max(peak_widths))
            dos = np.zeros(len(hist) - 1 + n_gauss)

            for index, width in zip(peaks, peak_widths.tolist()):
                sigma = width / 2.354
                for g in range(-n_gauss, n_gauss):
                    if index + g > 0:
                        dos[index + g] += hist[index] * math.exp(-(g**2) / (2 * sigma**2)) / (math.sqrt(2 * math.pi) * sigma)

        elif self._peak_func == "Lorentzian":
            n_lorentz = int(25.0 * np.max(peak_widths))
            dos = np.zeros(len(hist) - 1 + n_lorentz)

            for index, width in zip(peaks, peak_widths.tolist()):
                gamma_by_2 = width / 2
                for i in range(-n_lorentz, n_lorentz):
                    if index + i > 0:
                        dos[index + i] += hist[index] * gamma_by_2 / (i**2 + gamma_by_2**2) / math.pi

        return dos

    def _draw_sticks(self, peaks, dos_shape):
        """
        Draw a stick diagram for peaks.

        @param hist - array of counts for each bin
        @param peaks - the indicies of each non-zero point in the data
        @param dos_shape - shape of the DOS array with broadened peaks
        @return the y data
        """
        dos = np.zeros(dos_shape)
        stick_intensity = self.getProperty("StickHeight").value

        for index in peaks:
            dos[index] = stick_intensity

        return dos

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
        logger.debug("Computing partial DoS for: " + str(partial_ions))

        partial_workspaces = []
        total_workspace = None

        # Output each contribution to it's own workspace
        for ion_name, ions in partial_ions.items():
            partial_ws_name = self._out_ws_name + "_"
            partial_ws = self._compute_partial(ions, frequencies, eigenvectors, weights)

            # Set correct units on partial workspace
            partial_ws.setYUnit("(D/A)^2/amu")
            partial_ws.setYUnitLabel("Intensity")

            # Add the sample material to the workspace
            match = re.search(r"\d", ion_name)
            element_index = ion_name
            if match:
                element_index = ion_name[: match.start()]
            chemical, ws_suffix = self._parse_chemical_and_ws_name(ion_name, self._element_isotope[element_index])
            partial_ws_name += ws_suffix

            s_api.SetSampleMaterial(InputWorkspace=self._out_ws_name, ChemicalFormula=chemical)

            # Multiply intensity by scatttering cross section
            if self._scale_by_cross_section == "Incoherent":
                scattering_x_section = partial_ws.mutableSample().getMaterial().incohScatterXSection()
            elif self._scale_by_cross_section == "Coherent":
                scattering_x_section = partial_ws.mutableSample().getMaterial().cohScatterXSection()
            elif self._scale_by_cross_section == "Total":
                scattering_x_section = partial_ws.mutableSample().getMaterial().totalScatterXSection()

            if self._scale_by_cross_section != "None":
                scale_alg = self.createChildAlgorithm("Scale")
                scale_alg.setProperty("InputWorkspace", self._out_ws_name)
                scale_alg.setProperty("OutputWorkspace", self._out_ws_name)
                scale_alg.setProperty("Operation", "Multiply")
                scale_alg.setProperty("Factor", scattering_x_section)
                scale_alg.execute()

            rename_alg = self.createChildAlgorithm("RenameWorkspace")
            rename_alg.setProperty("InputWorkspace", self._out_ws_name)
            rename_alg.setProperty("OutputWorkspace", partial_ws_name)
            rename_alg.execute()
            partial_workspaces.append(rename_alg.getProperty("OutputWorkspace").value)

        total_workspace = self._out_ws_name + "_Total"

        # If there is more than one partial workspace need to sum first spectrum of all
        if len(partial_workspaces) > 1:
            initial_partial_ws = partial_workspaces[0]
            data_x = initial_partial_ws.dataX(0)
            dos_specs = np.zeros_like(initial_partial_ws.dataY(0))
            stick_specs = np.zeros_like(initial_partial_ws.dataY(0))

            for partial_ws in partial_workspaces:
                dos_specs += partial_ws.dataY(0)
                stick_specs += partial_ws.dataY(1)

            stick_specs[stick_specs > 0.0] = self.getProperty("StickHeight").value

            total_ws = self._create_dos_workspace(data_x, dos_specs, stick_specs, total_workspace)

            # Set correct units on total workspace
            total_ws.setYUnit("(D/A)^2/amu")
            total_ws.setYUnitLabel("Intensity")

        # Otherwise just repackage the WS we have as the total
        else:
            s_api.CloneWorkspace(InputWorkspace=partial_workspaces[0], OutputWorkspace=total_workspace)

        logger.debug("Partial workspaces: " + str(partial_workspaces))
        logger.debug("Summed workspace: " + str(total_workspace))

        return partial_workspaces, total_workspace

    def _parse_chemical_and_ws_name(self, ion_name, isotope):
        """
        @param ion_name     :: Name of the element used
        @param isotope      :: Isotope of the element
        @return The chemical formula of the element and isotope
                expected by SetSampleMaterial
                AND
                The expected suffix for the partial workspace
        """
        # Get the index of the element (if present)
        match = re.search(r"\d", ion_name)
        element_index = ""
        if match:
            element_index = "_" + ion_name[match.start() :]

        # If the chemical is a isotope
        if ":" in ion_name:
            chemical = ion_name.split(":")[0]
            # Parse isotope to rounded int
            chemical_formula = "(" + chemical + str(int(round(isotope))) + ")"
            ws_name_suffix = chemical + "(" + str(int(round(isotope))) + ")" + element_index
            return chemical_formula, ws_name_suffix
        # If the chemical has an index
        if match:
            chemical = ion_name[: match.start()]
            return chemical, chemical + element_index
        else:
            return ion_name, ion_name

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
            for mode in range(self._num_branches):
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
        return self._compute_DOS(frequencies, intensities, weights)

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
        out_ws = self._create_dos_workspace(data_x, dos, dos_sticks, self._out_ws_name)

        scale = self.getProperty("Scale").value
        if scale != 1:
            scale_alg = self.createChildAlgorithm("Scale")
            scale_alg.setProperty("InputWorkspace", out_ws)
            scale_alg.setProperty("OutputWorkspace", out_ws)
            scale_alg.setProperty("Operation", "Multiply")
            scale_alg.setProperty("Factor", scale)
            scale_alg.execute()

        bin_width = self.getProperty("BinWidth").value
        if bin_width != 1:
            x_min = out_ws.readX(0)[0] - (bin_width / 2.0)
            x_max = out_ws.readX(0)[-1] + (bin_width / 2.0)
            rebin_param = "%f, %f, %f" % (x_min, bin_width, x_max)
            out_ws = s_api.Rebin(Inputworkspace=out_ws, Params=rebin_param, OutputWorkspace=out_ws)

        return out_ws

    def _create_dos_workspace(self, data_x, dos, dos_sticks, out_name):
        ws = s_api.CreateWorkspace(
            DataX=data_x,
            DataY=np.ravel(np.array([dos, dos_sticks])),
            NSpec=2,
            VerticalAxisUnit="Text",
            VerticalAxisValues=[self._peak_func, "Stick"],
            OutputWorkspace=out_name,
            EnableLogging=False,
        )
        unitx = ws.getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Shift", "cm^-1")
        return ws

    def _compute_raman(self, frequencies, intensities, weights):
        """
        Compute Raman intensities

        @param frequencies - frequencies read from file
        @param intensities - raman intensities read from file
        @param weights - weights for each frequency block
        """
        # We only want to use the first set
        frequencies = frequencies[: self._num_branches]
        intensities = intensities[: self._num_branches]
        weights = weights[: self._num_branches]

        # Wavelength of the laser
        laser_wavelength = 514.5e-9
        # Planck's constant
        planck = scipy.constants.h
        # cm(-1) => K conversion
        cm1_to_K = scipy.constants.codata.value("inverse meter-kelvin relationship") * 100

        factor = (math.pow((2 * math.pi / laser_wavelength), 4) * planck) / (8 * math.pi**2 * 45) * 1e12
        x_sections = np.zeros(frequencies.size)

        # Use only the first set of frequencies and ignore small values
        zero_mask = np.where(frequencies > self._zero_threshold)
        frequency_x_sections = frequencies[zero_mask]
        intensity_x_sections = intensities[zero_mask]

        temperature = self.getProperty("Temperature").value

        bose_occ = 1.0 / (np.exp(cm1_to_K * frequency_x_sections / temperature) - 1)
        x_sections[zero_mask] = factor / frequency_x_sections * (1 + bose_occ) * intensity_x_sections

        return self._compute_DOS(frequencies, x_sections, weights)

    def _read_data_from_file(self, file_name):
        """
        Select the appropriate file parser and check data was successfully
        loaded from file.

        @param file_name - path to the file.
        @return tuple of the frequencies, ir and raman intensities and weights
        """
        from dos.load_castep import parse_castep_file
        from dos.load_phonon import parse_phonon_file

        ext = os.path.splitext(file_name)[1]

        if ext == ".phonon":
            record_eigenvectors = (
                self._calc_partial
                or (self._spec_type == "DOS" and self._scale_by_cross_section != "None")
                or self._spec_type == "BondAnalysis"
            )

            file_data, element_isotopes = parse_phonon_file(file_name, record_eigenvectors)
            self._element_isotope = element_isotopes
            self._num_ions = file_data["num_ions"]

        elif ext == ".castep":
            if len(self._ions_of_interest) > 0:
                raise ValueError("Cannot compute partial density of states from .castep files.")

            ir_or_raman = self._spec_type == "IR_Active" or self._spec_type == "Raman_Active"
            file_data = parse_castep_file(file_name, ir_or_raman)

        if file_data["frequencies"].size == 0:
            raise ValueError("Failed to load any frequencies from file.")

        return file_data


AlgorithmFactory.subscribe(SimulatedDensityOfStates)
