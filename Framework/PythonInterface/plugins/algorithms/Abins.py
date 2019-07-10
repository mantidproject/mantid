# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

try:
    import pathos.multiprocessing as mp

    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False

import numpy as np
import six
import os
import re

from mantid.api import mtd, AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty, \
    WorkspaceGroup
from mantid.api import WorkspaceFactory, AnalysisDataService

# noinspection PyProtectedMember
from mantid.simpleapi import CloneWorkspace, GroupWorkspaces, SaveAscii, Load, Scale
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty, Atom
import AbinsModules


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(PythonAlgorithm):
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
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _extracted_ab_initio_data = None

    def category(self):
        return "Simulation"

        # ----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates inelastic neutron scattering."

        # ----------------------------------------------------------------------------------------

    def PyInit(self):

        # Declare all properties
        self.declareProperty(name="AbInitioProgram",
                             direction=Direction.Input,
                             defaultValue="CASTEP",
                             validator=StringListValidator(["CASTEP", "CRYSTAL", "DMOL3", "GAUSSIAN"]),
                             doc="An ab initio program which was used for vibrational or phonon calculation.")

        self.declareProperty(FileProperty("VibrationalOrPhononFile", "",
                                          action=FileAction.Load,
                                          direction=Direction.Input,
                                          extensions=["phonon", "out", "outmol", "log", "LOG"]),
                             doc="File with the data from a vibrational or phonon calculation.")

        self.declareProperty(FileProperty("ExperimentalFile", "",
                                          action=FileAction.OptionalLoad,
                                          direction=Direction.Input,
                                          extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="TemperatureInKelvin",
                             direction=Direction.Input,
                             defaultValue=10.0,
                             doc="Temperature in K for which dynamical structure factor S should be calculated.")

        self.declareProperty(name="BinWidthInWavenumber", defaultValue=1.0, doc="Width of bins used during rebining.")

        self.declareProperty(name="Scale", defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="SampleForm",
                             direction=Direction.Input,
                             defaultValue="Powder",
                             validator=StringListValidator(AbinsModules.AbinsConstants.ALL_SAMPLE_FORMS),
                             # doc="Form of the sample: SingleCrystal or Powder.")
                             doc="Form of the sample: Powder.")

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             validator=StringListValidator(AbinsModules.AbinsConstants.ALL_INSTRUMENTS),
                             doc="Name of an instrument for which analysis should be performed.")

        self.declareProperty(StringArrayProperty("Atoms", Direction.Input),
                             doc="List of atoms to use to calculate partial S."
                                 "If left blank, workspaces with S for all types of atoms will be calculated.")

        self.declareProperty(name="SumContributions", defaultValue=False,
                             doc="Sum the partial dynamical structure factors into a single workspace.")

        self.declareProperty(name="ScaleByCrossSection", defaultValue='Incoherent',
                             validator=StringListValidator(['Total', 'Incoherent', 'Coherent']),
                             doc="Scale the partial dynamical structure factors by the scattering cross section.")

        self.declareProperty(name="QuantumOrderEventsNumber", defaultValue='1',
                             validator=StringListValidator(['1', '2', '3', '4']),
                             doc="Number of quantum order effects included in the calculation "
                                 "(1 -> FUNDAMENTALS, 2-> first overtone + FUNDAMENTALS + "
                                 "2nd order combinations, 3-> FUNDAMENTALS + first overtone + second overtone + 2nd "
                                 "order combinations + 3rd order combinations etc...)")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", '', Direction.Output),
                             doc="Name to give the output workspace.")

    def validateInputs(self):
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """

        input_file_validators = {"CASTEP": self._validate_castep_input_file,
                                 "CRYSTAL": self._validate_crystal_input_file,
                                 "DMOL3": self._validate_dmol3_input_file,
                                 "GAUSSIAN": self._validate_gaussian_input_file}

        issues = dict()

        temperature = self.getProperty("TemperatureInKelvin").value
        if temperature < 0:
            issues["TemperatureInKelvin"] = "Temperature must be positive."

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        ab_initio_program = self.getProperty("AbInitioProgram").value
        vibrational_or_phonon_data_filename = self.getProperty("VibrationalOrPhononFile").value
        output = input_file_validators[ab_initio_program](filename_full_path=vibrational_or_phonon_data_filename)
        bin_width = self.getProperty("BinWidthInWavenumber").value
        if not (isinstance(bin_width, float) and 1.0 <= bin_width <= 10.0):
            issues["BinWidthInWavenumber"] = ["Invalid bin width. Valid range is [1.0, 10.0] cm^-1"]

        if output["Invalid"]:
            issues["VibrationalOrPhononFile"] = output["Comment"]

        workspace_name = self.getPropertyValue("OutputWorkspace")
        # list of special keywords which cannot be used in the name of workspace
        forbidden_keywords = ["total"]
        if workspace_name in mtd:
            issues["OutputWorkspace"] = "Workspace with name " + workspace_name + " already in use; please give " \
                                                                                  "a different name for workspace."
        elif workspace_name == "":
            issues["OutputWorkspace"] = "Please specify name of workspace."
        for word in forbidden_keywords:

            if word in workspace_name:
                issues["OutputWorkspace"] = "Keyword: " + word + " cannot be used in the name of workspace."
                break

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
        ab_initio_loaders = {"CASTEP": AbinsModules.LoadCASTEP, "CRYSTAL": AbinsModules.LoadCRYSTAL,
                             "DMOL3": AbinsModules.LoadDMOL3, "GAUSSIAN": AbinsModules.LoadGAUSSIAN}
        rdr = ab_initio_loaders[self._ab_initio_program](input_ab_initio_filename=self._vibrational_or_phonon_data_file)
        ab_initio_data = rdr.get_formatted_data()
        prog_reporter.report("Vibrational/phonon data has been read.")

        # 3) calculate S
        s_calculator = AbinsModules.CalculateS.init(filename=self._vibrational_or_phonon_data_file,
                                                    temperature=self._temperature,
                                                    sample_form=self._sample_form, abins_data=ab_initio_data,
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
            prefix = AbinsModules.AbinsConstants.ATOM_PREFIX
            atom_symbols = [item for item in self._atoms if item[:len(prefix)] != prefix]
            if len(atom_symbols) != len(set(atom_symbols)):  # only different types
                raise ValueError("User atom selection (by symbol) contains repeated species. This is not permitted as "
                                 "Abins cannot create multiple workspaces with the same name.")

            numbered_atom_test = re.compile('^' + prefix + '_?(\d+)$')
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
                                     (prefix, atom_number, 1, num_atoms))

            # Final sanity check that everything in "atoms" field was understood
            if len(atom_symbols) + len(atom_numbers) < len(self._atoms):
                elements_report = " Symbols: " + ", ".join(atom_symbols) if len(atom_symbols) else ""
                numbers_report = " Numbers: " + ", ".join(atom_numbers) if len(atom_numbers) else ""
                raise ValueError("Not all user atom selections ('atoms' option) were understood." +
                                 elements_report + numbers_report)

        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # at the moment only types of atom, e.g, for  benzene three options -> 1) C, H;  2) C; 3) H
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

        # 7) add experimental data if available to the collection of workspaces
        if self._experimental_file != "":
            workspaces.insert(0, self._create_experimental_data_workspace().name())
            prog_reporter.report("Workspace with the experimental data has been constructed.")

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
        :type AbinsModules.SData.SData

        :returns: workspaces for list of atoms types, S for the particular type of atom
        """
        from AbinsModules.AbinsConstants import MASS_EPS, ONLY_ONE_MASS

        s_data_extracted = s_data.extract()

        # Create appropriately-shaped arrays to be used in-place by _atom_type_s - avoid repeated slow instantiation
        shape = [self._num_quantum_order_events]
        shape.extend(list(s_data_extracted["atom_0"]["s"]["order_1"].shape))
        s_atom_data = np.zeros(shape=tuple(shape), dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)
        temp_s_atom_data = np.copy(s_atom_data)

        num_atoms = len([key for key in s_data_extracted.keys() if "atom" in key])
        masses = self._get_masses_table(num_atoms)

        result = []

        if atoms_symbols is not None:
            for symbol in atoms_symbols:
                sub = (len(masses[symbol]) > ONLY_ONE_MASS or
                       abs(Atom(symbol=symbol).mass - masses[symbol][0]) > MASS_EPS)
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
            method of AbinsModules.SData.SData object.
        :type s_data: dict

        :param s_atom_data: helper array to accumulate S (outer loop over atoms); does not transport
            information but is used in-place to save on time instantiating large arrays. First dimension is quantum
            order; following dimensions should match arrays in s_data.
        :type s_atom_data: numpy.ndarray

        :param

        :returns: mantid workspaces of S for atom (total) and individual quantum orders
        :returntype: list of Workspace2D
        """
        atom_workspaces = []
        s_atom_data.fill(0.0)
        internal_atom_label = "atom_%s" % (atom_number - 1)
        output_atom_label = "%s_%d" % (AbinsModules.AbinsConstants.ATOM_PREFIX, atom_number)
        symbol = self._extracted_ab_initio_data[internal_atom_label]["symbol"]
        z_number = Atom(symbol=symbol).z_number

        for i, order in enumerate(range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                                        self._num_quantum_order_events + AbinsModules.AbinsConstants.S_LAST_INDEX)):

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
        from AbinsModules.AbinsConstants import MASS_EPS

        atom_workspaces = []
        s_atom_data.fill(0.0)

        element = Atom(symbol=element_symbol)

        for atom in range(num_atoms):
            if (self._extracted_ab_initio_data["atom_%s" % atom]["symbol"] == element_symbol and
                    abs(self._extracted_ab_initio_data["atom_%s" % atom]["mass"] - mass) < MASS_EPS):

                temp_s_atom_data.fill(0.0)

                for order in range(AbinsModules.AbinsConstants.FUNDAMENTALS,
                                   self._num_quantum_order_events + AbinsModules.AbinsConstants.S_LAST_INDEX):
                    order_indx = order - AbinsModules.AbinsConstants.PYTHON_INDEX_SHIFT
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
        :type AbinsModules.SData.SData

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
        if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
            # only FUNDAMENTALS
            if s_points.shape[0] == AbinsModules.AbinsConstants.FUNDAMENTALS:

                self._fill_s_1d_workspace(s_points=s_points[0], workspace=workspace, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            # total workspaces
            elif len(s_points.shape) == AbinsModules.AbinsConstants.ONE_DIMENSIONAL_SPECTRUM:

                self._fill_s_1d_workspace(s_points=s_points, workspace=workspace, protons_number=protons_number,
                                          nucleons_number=nucleons_number)

            # quantum order events (fundamentals  or  overtones + combinations for the given order)
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
        :param protons_number: number of protons in the given type fo atom
        :param nucleons_number: number of nucleons in the given type of atom
        :param s_points: dynamical factor for the given atom
        :param workspace: workspace to be filled with S
        """
        if protons_number is not None:
            s_points = s_points * self._scale * self._get_cross_section(protons_number=protons_number,
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
        self._set_workspace_units(wrk=workspace)

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
        total_workspace = self._out_ws_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].names()
        else:
            local_partial_workspaces = partial_workspaces

        if len(local_partial_workspaces) > 1:

            # get frequencies
            ws = mtd[local_partial_workspaces[0]]

            # initialize S
            s_atoms = np.zeros_like(ws.dataY(0))

            # collect all S
            for partial_ws in local_partial_workspaces:
                if self._instrument.get_name() in AbinsModules.AbinsConstants.ONE_DIMENSIONAL_INSTRUMENTS:
                    s_atoms += mtd[partial_ws].dataY(0)

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

    def _set_workspace_units(self, wrk=None):
        """
        Sets x and y units for a workspace.
        :param wrk: workspace which units should be set
        """
        mtd[wrk].getAxis(0).setUnit("DeltaE_inWavenumber")
        mtd[wrk].setYUnitLabel("S /Arbitrary Units")
        mtd[wrk].setYUnit("Arbitrary Units")

    def _check_advanced_parameter(self):
        """
        Checks if parameters from AbinsParameters.py are valid. If any parameter is invalid then RuntimeError is thrown
        with meaningful message.
        """

        message = " in AbinsParameters.py. "

        self._check_general_resolution(message)
        self._check_tosca_parameters(message)
        self._check_folder_names(message)
        self._check_rebining(message)
        self._check_threshold(message)
        self._check_chunk_size(message)
        self._check_threads(message)

    def _check_general_resolution(self, message_end=None):
        """
        Checks general parameters used in construction resolution functions.
        :param message_end: closing part of the error message.
        """
        # check fwhm
        fwhm = AbinsModules.AbinsParameters.fwhm
        if not (isinstance(fwhm, float) and 0.0 < fwhm < 10.0):
            raise RuntimeError("Invalid value of fwhm" + message_end)

        # check delta_width
        delta_width = AbinsModules.AbinsParameters.delta_width
        if not (isinstance(delta_width, float) and 0.0 < delta_width < 1.0):
            raise RuntimeError("Invalid value of delta_width" + message_end)

    def _check_tosca_parameters(self, message_end=None):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        final_energy = AbinsModules.AbinsParameters.tosca_final_neutron_energy
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = AbinsModules.AbinsParameters.tosca_cos_scattering_angle
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = AbinsModules.AbinsParameters.tosca_a
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_b = AbinsModules.AbinsParameters.tosca_b
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_c = AbinsModules.AbinsParameters.tosca_c
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

    def _check_folder_names(self, message_end=None):
        """
        Checks folders names.
        :param message_end: closing part of the error message.
        """
        folder_names = []
        ab_initio_group = AbinsModules.AbinsParameters.ab_initio_group
        if not isinstance(ab_initio_group, str) or ab_initio_group == "":
            raise RuntimeError("Invalid name for folder in which the ab initio data should be stored.")
        folder_names.append(ab_initio_group)

        powder_data_group = AbinsModules.AbinsParameters.powder_data_group
        if not isinstance(powder_data_group, str) or powder_data_group == "":
            raise RuntimeError("Invalid value of powder_data_group" + message_end)
        elif powder_data_group in folder_names:
            raise RuntimeError("Name for powder_data_group  already used by as name of another folder.")
        folder_names.append(powder_data_group)

        crystal_data_group = AbinsModules.AbinsParameters.crystal_data_group
        if not isinstance(crystal_data_group, str) or crystal_data_group == "":
            raise RuntimeError("Invalid value of crystal_data_group" + message_end)
        elif crystal_data_group in folder_names:
            raise RuntimeError("Name for crystal_data_group already used as a name of another folder.")

        s_data_group = AbinsModules.AbinsParameters.s_data_group
        if not isinstance(s_data_group, str) or s_data_group == "":
            raise RuntimeError("Invalid value of s_data_group" + message_end)
        elif s_data_group in folder_names:
            raise RuntimeError("Name for s_data_group already used as a name of another folder.")

    def _check_rebining(self, message_end=None):
        """
        Checks rebinning parameters.
        :param message_end: closing part of the error message.
        """
        pkt_per_peak = AbinsModules.AbinsParameters.pkt_per_peak
        if not (isinstance(pkt_per_peak, six.integer_types) and 1 <= pkt_per_peak <= 1000):
            raise RuntimeError("Invalid value of pkt_per_peak" + message_end)

        min_wavenumber = AbinsModules.AbinsParameters.min_wavenumber
        if not (isinstance(min_wavenumber, float) and min_wavenumber >= 0.0):
            raise RuntimeError("Invalid value of min_wavenumber" + message_end)

        max_wavenumber = AbinsModules.AbinsParameters.max_wavenumber
        if not (isinstance(max_wavenumber, float) and max_wavenumber > 0.0):
            raise RuntimeError("Invalid number of max_wavenumber" + message_end)

        if min_wavenumber > max_wavenumber:
            raise RuntimeError("Invalid energy window for rebinning.")

    def _check_threshold(self, message_end=None):
        """
        Checks threshold for frequencies.
        :param message_end: closing part of the error message.
        """
        freq_threshold = AbinsModules.AbinsParameters.frequencies_threshold
        if not (isinstance(freq_threshold, float) and freq_threshold >= 0.0):
            raise RuntimeError("Invalid value of frequencies_threshold" + message_end)

        # check s threshold
        s_absolute_threshold = AbinsModules.AbinsParameters.s_absolute_threshold
        if not (isinstance(s_absolute_threshold, float) and s_absolute_threshold > 0.0):
            raise RuntimeError("Invalid value of s_absolute_threshold" + message_end)

        s_relative_threshold = AbinsModules.AbinsParameters.s_relative_threshold
        if not (isinstance(s_relative_threshold, float) and s_relative_threshold > 0.0):
            raise RuntimeError("Invalid value of s_relative_threshold" + message_end)

    def _check_chunk_size(self, message_end=None):
        """
        Check optimal size of chunk
        :param message_end: closing part of the error message.
        """
        optimal_size = AbinsModules.AbinsParameters.optimal_size
        if not (isinstance(optimal_size, six.integer_types) and optimal_size > 0):
            raise RuntimeError("Invalid value of optimal_size" + message_end)

    def _check_threads(self, message_end=None):
        """
        Checks number of threads
        :param message_end: closing part of the error message.
        """
        if PATHOS_FOUND:
            threads = AbinsModules.AbinsParameters.threads
            if not (isinstance(threads, six.integer_types) and 1 <= threads <= mp.cpu_count()):
                raise RuntimeError("Invalid number of threads for parallelisation over atoms" + message_end)

    def _validate_ab_initio_file_extension(self, filename_full_path=None, expected_file_extension=None):
        """
        Checks consistency between name of ab initio program and extension.
        :param expected_file_extension: file extension
        :returns: dictionary with error message
        """
        ab_initio_program = self.getProperty("AbInitioProgram").value
        msg_err = "Invalid %s file. " % filename_full_path
        msg_rename = "Please rename your file and try again."

        # check  extension of a file
        found_filename_ext = os.path.splitext(filename_full_path)[1]
        if found_filename_ext.lower() != expected_file_extension:
            comment = "{}Output from ab initio program {} is expected." \
                      " The expected extension of file is .{}. Found: {}.{}".format(
                          msg_err, ab_initio_program, expected_file_extension, found_filename_ext, msg_rename)
            return dict(Invalid=True, Comment=comment)
        else:
            return dict(Invalid=False, Comment="")

    def _validate_dmol3_input_file(self, filename_full_path=None):
        """
        Method to validate input file for DMOL3 ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate DMOL3 file with vibrational data.")
        return self._validate_ab_initio_file_extension(filename_full_path=filename_full_path,
                                                       expected_file_extension=".outmol")

    def _validate_gaussian_input_file(self, filename_full_path=None):
        """
        Method to validate input file for GAUSSIAN ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate GAUSSIAN file with vibration data.")
        return self._validate_ab_initio_file_extension(filename_full_path=filename_full_path,
                                                       expected_file_extension=".log")

    def _validate_crystal_input_file(self, filename_full_path=None):
        """
        Method to validate input file for CRYSTAL ab initio program.
        :param filename_full_path: full path of a file to check.
        :returns: True if file is valid otherwise false.
        """
        logger.information("Validate CRYSTAL file with vibrational or phonon data.")
        return self._validate_ab_initio_file_extension(filename_full_path=filename_full_path,
                                                       expected_file_extension=".out")

    def _validate_castep_input_file(self, filename_full_path=None):
        """
        Check if ab initio input vibrational or phonon file has been produced by CASTEP. Currently the crucial
        keywords in the first few lines are checked (to be modified if a better validation is found...)
        :param filename_full_path: full path of a file to check
        :returns: Dictionary with two entries "Invalid", "Comment". Valid key can have two values: True/ False. As it
                  comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """
        logger.information("Validate CASTEP file with vibrational or phonon data.")
        msg_err = "Invalid %s file. " % filename_full_path
        output = self._validate_ab_initio_file_extension(filename_full_path=filename_full_path,
                                                         expected_file_extension=".phonon")
        if output["Invalid"]:
            return output

        # check a structure of the header part of file.
        # Here fortran convention is followed: case of letter does not matter
        with open(filename_full_path) as castep_file:

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(line, "beginheader"):  # first line is BEGIN header
                return dict(Invalid=True, Comment=msg_err + "The first line should be 'BEGIN header'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofions"):
                return dict(Invalid=True, Comment=msg_err + "The second line should include 'Number of ions'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofbranches"):
                return dict(Invalid=True, Comment=msg_err + "The third line should include 'Number of branches'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofwavevectors"):
                return dict(Invalid=True, Comment=msg_err + "The fourth line should include 'Number of wavevectors'.")

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line,
                                          pattern="frequenciesin"):
                return dict(Invalid=True, Comment=msg_err + "The fifth line should be 'Frequencies in'.")

        return output

    def _get_one_line(self, file_obj=None):
        """

        :param file_obj:  file object from which reading is done
        :returns: string containing one non empty line
        """
        line = file_obj.readline().replace(" ", "").lower()

        while line and line == "":
            line = file_obj.readline().replace(" ", "").lower()

        return line

    def _compare_one_line(self, one_line, pattern):
        """
        compares line in the the form of string with a pattern.
        :param one_line:  line in the for mof string to be compared
        :param pattern: string which should be present in the line after removing white spaces and setting all
                        letters to lower case
        :returns:  True is pattern present in the line, otherwise False
        """
        return one_line and pattern in one_line.replace(" ", "")

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """

        self._ab_initio_program = self.getProperty("AbInitioProgram").value
        self._vibrational_or_phonon_data_file = self.getProperty("VibrationalOrPhononFile").value
        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._temperature = self.getProperty("TemperatureInKelvin").value
        self._bin_width = self.getProperty("BinWidthInWavenumber").value
        self._scale = self.getProperty("Scale").value
        self._sample_form = self.getProperty("SampleForm").value

        instrument_name = self.getProperty("Instrument").value
        if instrument_name in AbinsModules.AbinsConstants.ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
            instrument_producer = AbinsModules.InstrumentProducer()
            self._instrument = instrument_producer.produce_instrument(name=self._instrument_name)
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value

        # conversion from str to int
        self._num_quantum_order_events = int(self.getProperty("QuantumOrderEventsNumber").value)

        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')
        self._calc_partial = (len(self._atoms) > 0)

        # user defined interval is exclusive with respect to
        # AbinsModules.AbinsParameters.min_wavenumber
        # AbinsModules.AbinsParameters.max_wavenumber
        # with bin width AbinsModules.AbinsParameters.bin_width
        step = self._bin_width
        start = AbinsModules.AbinsParameters.min_wavenumber + step / 2.0
        stop = AbinsModules.AbinsParameters.max_wavenumber + step / 2.0
        self._bins = np.arange(start=start, stop=stop, step=step, dtype=AbinsModules.AbinsConstants.FLOAT_TYPE)


AlgorithmFactory.subscribe(Abins)
