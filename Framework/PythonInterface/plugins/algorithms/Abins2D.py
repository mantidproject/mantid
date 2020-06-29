# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np
import re
from typing import Dict

from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceGroup
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces, SaveAscii, Load, Scale
from mantid.kernel import StringListValidator, Direction, Atom
import abins
from abins.abinsalgorithm import AbinsAlgorithm


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins2D(PythonAlgorithm, AbinsAlgorithm):
    _ab_initio_program = None
    _vibrational_or_phonon_data_file = None
    _temperature = None
    _bin_width = None
    _sample_form = None
    _instrument_name = None
    _atoms = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _extracted_ab_initio_data = None

    def category(self) -> str:
        return "Simulation"

    def summary(self) -> str:
        return "Calculates inelastic neutron scattering over 2D (q,ω) space."

    def PyInit(self) -> None:
        from abins.constants import TWO_DIMENSIONAL_INSTRUMENTS
        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TwoDMap",
                             validator=StringListValidator(TWO_DIMENSIONAL_INSTRUMENTS),
                             doc="Name of an instrument for which analysis should be performed.")

        # Declare special properties for 2D instrument
        self.declareProperty(name="Resolution",
                             direction=Direction.Input,
                             defaultValue=abins.parameters.instruments['TwoDMap']['resolution'],
                             doc="Width of broadening function")

        self.declareProperty(name="Angles",
                             direction=Direction.Input,
                             defaultValue="3.0, 140.0, 50")

    # 'TwoDMap': {
    #     'resolution': 0.1,  # Width of broadening function
    #     'q_size': 200,  # Number of q slices
    #     'e_init': [4100.0],  # Incident energies in cm-1
    #     'angles': np.arange(3.0, 140.0, 1).tolist(), # All measurement angles for direct sweeps
    #     },

    def validateInputs(self) -> Dict[str,str]:
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """

        issues = dict()
        issues = self.validate_common_inputs(issues)

        self._check_advanced_parameter()

        import numbers
        if not isinstance(self.getProperty("Resolution").value, numbers.Real):
            issues["Resolution"] = "Resolution must a real number"

        try:
            self._angle_string_to_list(self.getProperty("Angles").value)

        except ValueError:
            issues["Angles"] = ("Could not parse Angle input. Format should be "
                                "\"min, max, steps\" or \"angle1, angle2, angle3, ...\"")

        return issues

    def PyExec(self):
        from abins.constants import ATOM_PREFIX

        # 0) Create reporter to report progress
        steps = 9
        begin = 0
        end = 1.0
        prog_reporter = Progress(self, begin, end, steps)

        # 1) get input parameters from a user
        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        # 2) read ab initio data
        ab_initio_data = abins.AbinsData.from_calculation_data(self._vibrational_or_phonon_data_file,
                                                               self._ab_initio_program)
        prog_reporter.report("Vibrational/phonon data has been read.")

        # 3) calculate S
        s_calculator = abins.SCalculatorFactory.init(filename=self._vibrational_or_phonon_data_file,
                                                     temperature=self._temperature,
                                                     sample_form=self._sample_form,
                                                     abins_data=ab_initio_data,
                                                     instrument=self._instrument,
                                                     quantum_order_num=self._num_quantum_order_events,
                                                     bin_width=self._bin_width)
        s_data = s_calculator.get_formatted_data()
        prog_reporter.report("Dynamical structure factors have been determined.")

        # 4) get atoms for which S should be plotted
        self._extracted_ab_initio_data = ab_initio_data.get_atoms_data().extract()
        num_atoms = len(self._extracted_ab_initio_data)
        all_atms_smbls = list(set([self._extracted_ab_initio_data["atom_%s" % atom]["symbol"]
                                   for atom in range(num_atoms)]))
        all_atms_smbls.sort()

        if len(self._atoms) == 0:  # case: all atoms
            atom_symbols = all_atms_smbls
            atom_numbers = []
        else:  # case selected atoms
            # Specific atoms are identified with prefix and integer index, e.g 'atom_5'. Other items are element symbols
            # A regular expression match is used to make the underscore separator optional and check the index format
            atom_symbols = [item for item in self._atoms if item[:len(prefix)] != ATOM_PREFIX]
            if len(atom_symbols) != len(set(atom_symbols)):  # only different types
                raise ValueError("User atom selection (by symbol) contains repeated species. This is not permitted as "
                                 "Abins cannot create multiple workspaces with the same name.")

            numbered_atom_test = re.compile('^' + ATOM_PREFIX + r'_?(\d+)$')
            atom_numbers = [numbered_atom_test.findall(item) for item in self._atoms]  # Matches will be lists of str
            atom_numbers = [int(match[0]) for match in atom_numbers if match]  # Remove empty matches, cast rest to int

            if len(atom_numbers) != len(set(atom_numbers)):
                raise ValueError("User atom selection (by number) contains repeated atom. This is not permitted as Abins"
                                 " cannot create multiple workspaces with the same name.")

            for atom_symbol in atom_symbols:
                if atom_symbol not in all_atms_smbls:
                    raise ValueError("User defined atom selection (by element) '%s': not present in the system." %
                                     atom_symbol)

            for atom_number in atom_numbers:
                if atom_number < 1 or atom_number > num_atoms:
                    raise ValueError("Invalid user atom selection (by number) '%s%s': out of range (%s - %s)" %
                                     (ATOM_PREFIX, atom_number, 1, num_atoms))

            # Final sanity check that everything in "atoms" field was understood
            if len(atom_symbols) + len(atom_numbers) < len(self._atoms):
                elements_report = " Symbols: " + ", ".join(atom_symbols) if len(atom_symbols) else ""
                numbers_report = " Numbers: " + ", ".join(atom_numbers) if len(atom_numbers) else ""
                raise ValueError("Not all user atom selections ('atoms' option) were understood."
                                 + elements_report + numbers_report)

        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # 5) create workspaces for atoms in interest
        workspaces = []
        if self._sample_form == "Powder":
            workspaces.extend(self._create_partial_s_per_type_workspaces(atoms_symbols=atom_symbols, s_data=s_data))
            workspaces.extend(self._create_partial_s_per_type_workspaces(atom_numbers=atom_numbers, s_data=s_data))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            total_atom_workspaces = []
            for ws in workspaces:
                if "total" in ws:
                    total_atom_workspaces.append(ws)
            total_workspace = self._create_total_workspace(partial_workspaces=total_atom_workspaces)
            workspaces.insert(0, total_workspace)
            prog_reporter.report("Workspace with total S has been constructed.")

        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        num_workspaces = mtd[self._out_ws_name].getNumberOfEntries()
        for wrk_num in range(num_workspaces):
            wrk = mtd[self._out_ws_name].getItem(wrk_num)
            SaveAscii(InputWorkspace=Scale(wrk, 1.0 / self._bin_width, "Multiply"),
                      Filename=wrk.name() + ".dat", Separator="Space", WriteSpectrumID=False)
        prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _get_masses_table(self, num_atoms):
        """
        Collect masses associated with each element in self._extracted_ab_initio_data

        :param num_atoms: Number of atoms in the system. (Saves time working out iteration.)
        :type: int

        :returns: Mass data in form ``{el1: [m1, ...], ... }``
        """
        masses = {}
        for i in range(num_atoms):
            symbol = self._extracted_ab_initio_data["atom_%s" % i]["symbol"]
            mass = self._extracted_ab_initio_data["atom_%s" % i]["mass"]
            if symbol not in masses:
                masses[symbol] = set()
            masses[symbol].add(mass)

        # convert set to list to fix order
        for s in masses:
            masses[s] = sorted(list(set(masses[s])))

        return masses

    def _create_workspaces(self, atoms_symbols=None, atom_numbers=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Creates both partial and total workspaces for given types of atoms.

        :param atoms_symbols: atom types (i.e. element symbols) for which S should be created.
        :type iterable of str:

        :param atom_numbers:
            indices of individual atoms for which S should be created. (One-based numbering; 1 <= I <= NUM_ATOMS)
        :type iterable of int:

        :param s_data: dynamical factor data
        :type abins.sdata.SData

        :returns: workspaces for list of atoms types, S for the particular type of atom
        """
        from abins.constants import MASS_EPS, ONLY_ONE_MASS

        s_data_extracted = s_data.extract()

        # Create appropriately-shaped arrays to be used in-place by _atom_type_s - avoid repeated slow instantiation
        shape = [self._num_quantum_order_events]
        shape.extend(list(s_data_extracted["atom_0"]["s"]["order_1"].shape))
        s_atom_data = np.zeros(shape=tuple(shape), dtype=abins.constants.FLOAT_TYPE)
        temp_s_atom_data = np.copy(s_atom_data)

        num_atoms = len([key for key in s_data_extracted.keys() if "atom" in key])
        masses = self._get_masses_table(num_atoms)

        result = []

        if atoms_symbols is not None:
            for symbol in atoms_symbols:
                sub = (len(masses[symbol]) > ONLY_ONE_MASS
                       or abs(Atom(symbol=symbol).mass - masses[symbol][0]) > MASS_EPS)
                for m in masses[symbol]:
                    result.extend(self._atom_type_s(num_atoms=num_atoms, mass=m, s_data_extracted=s_data_extracted,
                                                    element_symbol=symbol, temp_s_atom_data=temp_s_atom_data,
                                                    s_atom_data=s_atom_data, substitution=sub))
        if atom_numbers is not None:
            for atom_number in atom_numbers:
                result.extend(self._atom_number_s(atom_number=atom_number, s_data_extracted=s_data_extracted,
                                                  s_atom_data=s_atom_data))
        return result

    def _atom_number_s(self, atom_number=None, s_data_extracted=None, s_atom_data=None):
        """
        Helper function for calculating S for the given atomic index

        :param atom_number: One-based index of atom in s_data e.g. 1 to select first element 'atom_1'
        :type atom_number: int

        :param s_data_extracted: Collection of precalculated S for all atoms and quantum orders, obtained from extract()
            method of abins.sdata.SData object.
        :type s_data: dict

        :param s_atom_data: helper array to accumulate S (outer loop over atoms); does not transport
            information but is used in-place to save on time instantiating large arrays. First dimension is quantum
            order; following dimensions should match arrays in s_data.
        :type s_atom_data: numpy.ndarray

        :param

        :returns: mantid workspaces of S for atom (total) and individual quantum orders
        :returntype: list of Workspace2D
        """
        from abins.constants import ATOM_PREFIX, FUNDAMENTALS, S_LAST_INDEX

        atom_workspaces = []
        s_atom_data.fill(0.0)
        internal_atom_label = "atom_%s" % (atom_number - 1)
        output_atom_label = "%s_%d" % (ATOM_PREFIX, atom_number)
        symbol = self._extracted_ab_initio_data[internal_atom_label]["symbol"]
        z_number = Atom(symbol=symbol).z_number

        for i, order in enumerate(range(FUNDAMENTALS,
                                        self._num_quantum_order_events + S_LAST_INDEX)):

            s_atom_data[i] = s_data_extracted[internal_atom_label]["s"]["order_%s" % order]

        total_s_atom_data = np.sum(s_atom_data, axis=0)

        atom_workspaces = []
        atom_workspaces.append(self._create_workspace(atom_name=output_atom_label,
                                                      s_points=np.copy(total_s_atom_data),
                                                      optional_name="_total", protons_number=z_number))
        atom_workspaces.append(self._create_workspace(atom_name=output_atom_label,
                                                      s_points=np.copy(s_atom_data),
                                                      protons_number=z_number))
        return atom_workspaces

    def _atom_type_s(self, num_atoms=None, mass=None, s_data_extracted=None, element_symbol=None, temp_s_atom_data=None,
                     s_atom_data=None, substitution=None):
        """
        Helper function for calculating S for the given type of atom

        :param num_atoms: number of atoms in the system
        :param s_data_extracted: data with all S
        :param element_symbol: label for the type of atom
        :param temp_s_atom_data: helper array to accumulate S (inner loop over quantum order); does not transport
            information but is used in-place to save on time instantiating large arrays.
        :param s_atom_data: helper array to accumulate S (outer loop over atoms); does not transport
            information but is used in-place to save on time instantiating large arrays.
        :param substitution: True if isotope substitution and False otherwise
        """
        from abins.constants import FUNDAMENTALS, MASS_EPS, PYTHON_INDEX_SHIFT, S_LAST_INDEX

        atom_workspaces = []
        s_atom_data.fill(0.0)

        element = Atom(symbol=element_symbol)

        for atom in range(num_atoms):
            if (self._extracted_ab_initio_data["atom_%s" % atom]["symbol"] == element_symbol
                    and abs(self._extracted_ab_initio_data["atom_%s" % atom]["mass"] - mass) < MASS_EPS):

                temp_s_atom_data.fill(0.0)

                for order in range(FUNDAMENTALS,
                                   self._num_quantum_order_events + S_LAST_INDEX):
                    order_indx = order - PYTHON_INDEX_SHIFT
                    temp_s_order = s_data_extracted["atom_%s" % atom]["s"]["order_%s" % order]
                    temp_s_atom_data[order_indx] = temp_s_order

                s_atom_data += temp_s_atom_data  # sum S over the atoms of the same type

        total_s_atom_data = np.sum(s_atom_data, axis=0)

        nucleons_number = int(round(mass))

        if substitution:

            atom_workspaces.append(self._create_workspace(atom_name=str(nucleons_number) + element_symbol,
                                                          s_points=np.copy(total_s_atom_data),
                                                          optional_name="_total", protons_number=element.z_number,
                                                          nucleons_number=nucleons_number))
            atom_workspaces.append(self._create_workspace(atom_name=str(nucleons_number) + element_symbol,
                                                          s_points=np.copy(s_atom_data),
                                                          protons_number=element.z_number,
                                                          nucleons_number=nucleons_number))
        else:

            atom_workspaces.append(self._create_workspace(atom_name=element_symbol,
                                                          s_points=np.copy(total_s_atom_data),
                                                          optional_name="_total", protons_number=element.z_number))
            atom_workspaces.append(self._create_workspace(atom_name=element_symbol,
                                                          s_points=np.copy(s_atom_data),
                                                          protons_number=element.z_number))

        return atom_workspaces

    def _create_partial_s_per_type_workspaces(self, atoms_symbols=None, atom_numbers=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Each workspace stores quantum order events for S for the given
        type of atom. It also stores total workspace for the given type of atom.

        :param atoms_symbols: atom types (i.e. element symbols) for which S should be created.
        :type iterable of str:

        :param atom_numbers: indices of individual atoms for which S should be created
        :type iterable of int:

        :param s_data: dynamical factor data
        :type abins.sdata.SData

        :returns: workspaces for list of atoms types, each workspace contains  quantum order events of
                 S for the particular atom type
        """

        return self._create_workspaces(atoms_symbols=atoms_symbols, atom_numbers=atom_numbers, s_data=s_data)

    def _fill_s_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        """
        Puts S into workspace(s).

        :param s_points: dynamical factor for the given atom
        :param workspace:  workspace to be filled with S
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        """
        from abins.constants import FUNDAMENTALS, ONE_DIMENSIONAL_INSTRUMENTS, TWO_DIMENSIONAL_INSTRUMENTS
        if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
            raise NotImplementedError()

        elif self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:

            # only FUNDAMENTALS [data is 3d with length 1 in axis 0]
            if s_points.shape[0] == FUNDAMENTALS:
                self._fill_s_2d_workspace(s_points=s_points[0], workspace=workspace, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            # total workspaces [data is 2d array of S]
            elif s_points.shape[0] == abins.parameters.instruments[self._instrument.get_name()]['q_size']:
                self._fill_s_2d_workspace(s_points=s_points, workspace=workspace, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            # Multiple quantum order events [data is 3d table of S using axis 0 for quantum orders]
            else:
                dim = s_points.shape[0]
                partial_wrk_names = []

                for n in range(dim):
                    seed = "quantum_event_%s" % (n + 1)
                    wrk_name = workspace + "_" + seed
                    partial_wrk_names.append(wrk_name)

                    self._fill_s_2d_workspace(s_points=s_points[n], workspace=wrk_name, protons_number=protons_number,
                                              nucleons_number=nucleons_number)

                GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _fill_s_2d_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        from abins.constants import Q_BEGIN, Q_END
        from mantid.api import NumericAxis

        if protons_number is not None:
            s_points = s_points *  self._get_cross_section(protons_number=protons_number,
                                                           nucleons_number=nucleons_number)

        n_q_bins, n_freq_bins = s_points.shape

        wrk = WorkspaceFactory.create("Workspace2D", NVectors=n_freq_bins, XLength=n_q_bins + 1, YLength=n_q_bins)

        freq_axis = NumericAxis.create(n_freq_bins)

        q_size = abins.parameters.instruments[self._instrument.get_name()]['q_size']
        q_bins = np.linspace(start=Q_BEGIN, stop=Q_END, num=q_size + 1)

        freq_offset = (self._bins[1] - self._bins[0]) / 2
        for i, freq in enumerate(self._bins[1:]):
            wrk.setX(i, q_bins)
            wrk.setY(i, s_points[:, i].T)
            freq_axis.setValue(i, freq + freq_offset)
        freq_axis.setUnit("Energy_inWavenumber")
        wrk.replaceAxis(1, freq_axis)

        AnalysisDataService.addOrReplace(workspace, wrk)

        self._set_workspace_units(wrk=workspace, layout="2D")

    def _get_cross_section(self, protons_number=None, nucleons_number=None):
        """
        Calculates cross section for the given element.
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        :returns: cross section for that element
        """
        if nucleons_number is not None:
            try:
                atom = Atom(a_number=nucleons_number, z_number=protons_number)
            # isotopes are not implemented for all elements so use different constructor in that cases
            except RuntimeError:
                atom = Atom(z_number=protons_number)
        else:
            atom = Atom(z_number=protons_number)

        cross_section = None
        if self._scale_by_cross_section == 'Incoherent':
            cross_section = atom.neutron()["inc_scatt_xs"]
        elif self._scale_by_cross_section == 'Coherent':
            cross_section = atom.neutron()["coh_scatt_xs"]
        elif self._scale_by_cross_section == 'Total':
            cross_section = atom.neutron()["tot_scatt_xs"]

        return cross_section

    def _create_total_workspace(self, partial_workspaces=None):
        """
        Sets workspace with total S.
        :param partial_workspaces: list of workspaces which should be summed up to obtain total workspace
        :returns: workspace with total S from partial_workspaces
                """
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS, TWO_DIMENSIONAL_INSTRUMENTS
        total_workspace = self._out_ws_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].names()
        else:
            local_partial_workspaces = partial_workspaces

        if len(local_partial_workspaces) > 1:

            # get frequencies
            ws = mtd[local_partial_workspaces[0]]

            # initialize S
            if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
                s_atoms = np.zeros_like(ws.dataY(0))
            if self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
                n_q = abins.parameters.instruments[self._instrument.get_name()]['q_size']
                n_energy_bins = ws.getDimension(1).getNBins()
                s_atoms = np.zeros([n_q, n_energy_bins])

            # collect all S
            for partial_ws in local_partial_workspaces:
                if self._instrument.get_name() in ONE_DIMENSIONAL_INSTRUMENTS:
                    s_atoms += mtd[partial_ws].dataY(0)
                elif self._instrument.get_name() in TWO_DIMENSIONAL_INSTRUMENTS:
                    for i in range(n_energy_bins):
                        s_atoms[:, i] += mtd[partial_ws].dataY(i)

            # create workspace with S
            self._fill_s_workspace(s_atoms, total_workspace)

        # # Otherwise just repackage the workspace we have as the total
        else:
            CloneWorkspace(InputWorkspace=local_partial_workspaces[0], OutputWorkspace=total_workspace)

        return total_workspace

    def _create_workspace(self, atom_name=None, s_points=None, optional_name="", protons_number=None,
                          nucleons_number=None):
        """
        Creates workspace for the given frequencies and s_points with S data. After workspace is created it is rebined,
        scaled by cross-section factor and optionally multiplied by the user defined scaling factor.


        :param atom_name: symbol of atom for which workspace should be created
        :param s_points: S(Q, omega)
        :param optional_name: optional part of workspace name
        :returns: workspace for the given frequency and S data
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        """

        ws_name = self._out_ws_name + "_" + atom_name + optional_name
        self._fill_s_workspace(s_points=s_points, workspace=ws_name, protons_number=protons_number,
                               nucleons_number=nucleons_number)
        return ws_name

    def _set_workspace_units(self, wrk=None, layout='1D'):
        """
        Sets x and y units for a workspace.
        :param wrk: workspace which units should be set
        :param layout: layout of data in Workspace2D.

            - '1D' is a typical indirect spectrum, with energy transfer on Axis
              0 (X), S on Axis 1 (Y)
            - '2D' is a 2D S(q,omega) map with momentum transfer on Axis 0 (X),
              S on Axis 1 and energy transfer on Axis 2
        """

        if layout == '1D':
            mtd[wrk].getAxis(0).setUnit("DeltaE_inWavenumber")
            mtd[wrk].setYUnitLabel("S / Arbitrary Units")
            mtd[wrk].setYUnit("Arbitrary Units")
        elif layout == '2D':
            mtd[wrk].getAxis(0).setUnit("MomentumTransfer")
            mtd[wrk].setYUnitLabel("S / Arbitrary Units")
            mtd[wrk].setYUnit("Arbitrary Units")
            mtd[wrk].getAxis(1).setUnit("DeltaE_inWavenumber")
        else:
            raise ValueError('Unknown data/units layout "{}"'.format(layout))

    def _check_advanced_parameter(self):
        """
        Checks if parameters from abins.parameters are valid. If any parameter
        is invalid then RuntimeError is thrown with meaningful message.
        """

        message = " in scripts/abins/parameters.py. "

        self._check_common_advanced_parameters(message)
        self._check_2d_parameters(message)

    def _check_2d_parameters(self, message_end=None):
        # check 2D resolution
        resolution_2d = abins.parameters.instruments['TwoDMap']['resolution']
        if not (isinstance(resolution_2d, float) and resolution_2d > 0):
            raise RuntimeError("Invalid value of abins.parameters"
                               ".instruments['TwoDMap']['resolution']"
                               + message_end)

    @staticmethod
    def _angle_string_to_list(angle_string):
        angle_string = angle_string.strip()
        if angle_string[0] == '[' and angle_string[-1] == ']':
            angles = list(map(float, angle_string[1:-1].split(',')))
        else:
            linspace_spec = list(map(float, angle_string.split(',')))
            if len(linspace_spec) != 3:
                raise ValueError()

            angles = np.linspace(*linspace_spec).tolist()

        return angles

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        self._get_common_properties()

        # Sampling mesh is determined by
        # abins.parameters.sampling['min_wavenumber']
        # abins.parameters.sampling['max_wavenumber']
        # and abins.parameters.sampling['bin_width']
        step = self._bin_width
        start = abins.parameters.sampling['min_wavenumber']
        stop = abins.parameters.sampling['max_wavenumber'] + step
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=abins.constants.FLOAT_TYPE)

        instrument_params = abins.parameters.instruments[self._instrument.get_name()]

        instrument_params['resolution'] = self.getProperty("Resolution").value
        instrument_params['angles'] = self._angle_string_to_list(self.getProperty("Angles").value)


AlgorithmFactory.subscribe(Abins2D)
