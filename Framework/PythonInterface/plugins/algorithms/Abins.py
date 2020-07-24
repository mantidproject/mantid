# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import numpy as np

from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceGroup
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces, Load
from mantid.kernel import StringListValidator, Direction, Atom
import abins
from abins.abinsalgorithm import AbinsAlgorithm


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(PythonAlgorithm, AbinsAlgorithm):
    _ab_initio_program = None
    _vibrational_or_phonon_data_file = None
    _experimental_file = None
    _temperature = None
    _bin_width = None
    _scale = None
    _sample_form = None
    _instrument_name = None
    _atoms = None
    _sum_contributions = None
    _save_ascii = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _atoms_data = None

    def category(self):
        return "Simulation"

    def summary(self):
        return "Calculates inelastic neutron scattering against 1-D ω axis."

    def PyInit(self):
        from abins.constants import ONE_DIMENSIONAL_INSTRUMENTS
        # Declare properties for all Abins Algorithms
        self.declare_common_properties()

        # Declare properties specific to 1D
        self.declareProperty(FileProperty("ExperimentalFile", "",
                                          action=FileAction.OptionalLoad,
                                          direction=Direction.Input,
                                          extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="Scale", defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             validator=StringListValidator(ONE_DIMENSIONAL_INSTRUMENTS),
                             doc="Name of an instrument for which analysis should be performed.")

    def validateInputs(self):
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """
        issues = dict()
        issues = self.validate_common_inputs(issues)

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        self._check_advanced_parameter()

        return issues

    def PyExec(self):
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
        self._atoms_data = ab_initio_data.get_atoms_data()

        atom_numbers, atom_symbols = self.get_atom_selection(atoms_data=self._atoms_data,
                                                             selection=self._atoms)

        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # 5) create workspaces for atoms in interest
        workspaces = []
        if self._sample_form == "Powder":
            workspaces.extend(self._create_partial_s_per_type_workspaces(atoms_symbols=atom_symbols, s_data=s_data))
            workspaces.extend(self._create_partial_s_per_type_workspaces(atom_numbers=atom_numbers, s_data=s_data))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            self.create_total_workspace(workspaces)
            prog_reporter.report("Workspace with total S has been constructed.")

        # 7) add experimental data if available to the collection of workspaces
        if self._experimental_file != "":
            workspaces.insert(0, self._create_experimental_data_workspace().name())
            prog_reporter.report("Workspace with the experimental data has been constructed.")

        GroupWorkspaces(InputWorkspaces=workspaces, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        if self._save_ascii:
            self.write_workspaces_to_ascii()
            prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _create_workspaces(self, atoms_symbols=None, atom_numbers=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Creates both partial and total workspaces for given types of atoms.

        :param atoms_symbols: atom types (i.e. element symbols) for which S should be created.
        :type iterable of str:

        :param atom_numbers:
            indices of individual atoms for which S should be created. (One-based numbering; 1 <= I <= NUM_ATOMS)
        :type iterable of int:

        :param s_data: dynamical factor data
        :type abins.SData

        :returns: workspaces for list of atoms types, S for the particular type of atom
        """
        from abins.constants import FLOAT_TYPE, MASS_EPS, ONLY_ONE_MASS

        # Create appropriately-shaped arrays to be used in-place by _atom_type_s - avoid repeated slow instantiation
        shape = [self._num_quantum_order_events]
        shape.extend(list(s_data[0]["order_1"].shape))
        s_atom_data = np.zeros(shape=tuple(shape), dtype=FLOAT_TYPE)
        temp_s_atom_data = np.copy(s_atom_data)

        num_atoms = len(s_data)
        masses = self.get_masses_table(num_atoms)

        result = []

        if atoms_symbols is not None:
            for symbol in atoms_symbols:
                sub = (len(masses[symbol]) > ONLY_ONE_MASS
                       or abs(Atom(symbol=symbol).mass - masses[symbol][0]) > MASS_EPS)
                for m in masses[symbol]:
                    result.extend(self._atom_type_s(num_atoms=num_atoms, mass=m, s_data=s_data,
                                                    element_symbol=symbol, temp_s_atom_data=temp_s_atom_data,
                                                    s_atom_data=s_atom_data, substitution=sub))
        if atom_numbers is not None:
            for atom_number in atom_numbers:
                result.extend(self._atom_number_s(atom_number=atom_number, s_data=s_data,
                                                  s_atom_data=s_atom_data))
        return result

    def _atom_number_s(self, atom_number=None, s_data=None, s_atom_data=None):
        """
        Helper function for calculating S for the given atomic index

        :param atom_number: One-based index of atom in s_data e.g. 1 to select first element 'atom_1'
        :type atom_number: int

        :param s_data: Precalculated S for all atoms and quantum orders
        :type s_data: abins.SData

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
        output_atom_label = "%s_%d" % (ATOM_PREFIX, atom_number)
        symbol = self._atoms_data[atom_number - 1]["symbol"]
        z_number = Atom(symbol=symbol).z_number

        for i, order in enumerate(range(FUNDAMENTALS, self._num_quantum_order_events + S_LAST_INDEX)):
            s_atom_data[i] = s_data[atom_number - 1]["order_%s" % order]

        total_s_atom_data = np.sum(s_atom_data, axis=0)

        atom_workspaces = []
        atom_workspaces.append(self._create_workspace(atom_name=output_atom_label,
                                                      s_points=np.copy(total_s_atom_data),
                                                      optional_name="_total", protons_number=z_number))
        atom_workspaces.append(self._create_workspace(atom_name=output_atom_label,
                                                      s_points=np.copy(s_atom_data),
                                                      protons_number=z_number))
        return atom_workspaces

    def _atom_type_s(self, num_atoms=None, mass=None, s_data=None, element_symbol=None, temp_s_atom_data=None,
                     s_atom_data=None, substitution=None):
        """
        Helper function for calculating S for the given type of atom

        :param num_atoms: number of atoms in the system
        :param s_data: Precalculated S for all atoms and quantum orders
        :type s_data: abins.SData
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

        for atom_index in range(num_atoms):
            if (self._atoms_data[atom_index]["symbol"] == element_symbol
                    and abs(self._atoms_data[atom_index]["mass"] - mass) < MASS_EPS):

                temp_s_atom_data.fill(0.0)

                for order in range(FUNDAMENTALS, self._num_quantum_order_events + S_LAST_INDEX):
                    order_indx = order - PYTHON_INDEX_SHIFT
                    temp_s_order = s_data[atom_index]["order_%s" % order]
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
        :type abins.SData

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

        from abins.constants import (FUNDAMENTALS, ONE_DIMENSIONAL_INSTRUMENTS, ONE_DIMENSIONAL_SPECTRUM)
        if self._instrument.get_name() not in ONE_DIMENSIONAL_INSTRUMENTS:
            raise ValueError("Instrument {self._instrument_name} is not supported by this version of Abins")

        # only FUNDAMENTALS [data is 2d with one row]
        if s_points.shape[0] == FUNDAMENTALS:
            self._fill_s_1d_workspace(s_points=s_points[0], workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # total workspaces [data is 1d vector]
        elif len(s_points.shape) == ONE_DIMENSIONAL_SPECTRUM:
            self._fill_s_1d_workspace(s_points=s_points, workspace=workspace, protons_number=protons_number,
                                      nucleons_number=nucleons_number)

        # quantum order events (fundamentals  or  overtones + combinations for the given order)
        # [data is 2d table of S with a row for each quantum order]
        else:
            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                seed = "quantum_event_%s" % (n + 1)
                wrk_name = workspace + "_" + seed
                partial_wrk_names.append(wrk_name)

                self._fill_s_1d_workspace(s_points=s_points[n], workspace=wrk_name, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            GroupWorkspaces(InputWorkspaces=partial_wrk_names, OutputWorkspace=workspace)

    def _fill_s_1d_workspace(self, s_points=None, workspace=None, protons_number=None, nucleons_number=None):
        """
        Puts 1D S into workspace.
        :param protons_number: number of protons in the given type of atom
        :param nucleons_number: number of nucleons in the given type of atom
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        """
        if protons_number is not None:
            s_points = s_points * self._scale * self.get_cross_section(protons_number=protons_number,
                                                                       nucleons_number=nucleons_number)

        dim = 1
        length = s_points.size
        wrk = WorkspaceFactory.create("Workspace2D", NVectors=dim, XLength=length + 1, YLength=length)
        for i in range(dim):
            wrk.getSpectrum(i).setDetectorID(i + 1)
        wrk.setX(0, self._bins)
        wrk.setY(0, s_points)
        AnalysisDataService.addOrReplace(workspace, wrk)

        # Set correct units on workspace
        self._set_workspace_units(wrk=workspace, layout="1D")

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

    def _create_experimental_data_workspace(self):
        """
        Loads experimental data into workspaces.
        :returns: workspace with experimental data
        """
        experimental_wrk = Load(self._experimental_file)
        self._set_workspace_units(wrk=experimental_wrk.name())

        return experimental_wrk

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
        Checks if parameters from abins.parameters are valid. If any parameter is invalid then RuntimeError is thrown
        with meaningful message.
        """

        message = " in abins.parameters. "

        self._check_common_advanced_parameters(message)

        self._check_tosca_parameters(message)
        self._check_2d_parameters(message)

    def _check_tosca_parameters(self, message_end=None):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        tosca_parameters = abins.parameters.instruments['TOSCA']
        final_energy = tosca_parameters['final_neutron_energy']
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = tosca_parameters['cos_scattering_angle']
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = tosca_parameters['a']
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

        resolution_const_b = tosca_parameters['b']
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

        resolution_const_c = tosca_parameters['c']
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)"
                               + message_end)

    def _check_2d_parameters(self, message_end=None):
        # check 2D resolution
        resolution_2d = abins.parameters.instruments['TwoDMap']['resolution']
        if not (isinstance(resolution_2d, float) and resolution_2d > 0):
            raise RuntimeError("Invalid value of AbinsParameters"
                               ".instruments['TwoDMap']['resolution']"
                               + message_end)

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """
        from abins.constants import FLOAT_TYPE

        self.get_common_properties()

        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._scale = self.getProperty("Scale").value

        # Sampling mesh is determined by
        # abins.parameters.sampling['min_wavenumber']
        # abins.parameters.sampling['max_wavenumber']
        # and abins.parameters.sampling['bin_width']
        step = self._bin_width
        start = abins.parameters.sampling['min_wavenumber']
        stop = abins.parameters.sampling['max_wavenumber'] + step
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=FLOAT_TYPE)


AlgorithmFactory.subscribe(Abins)
