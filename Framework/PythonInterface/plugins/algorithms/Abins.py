from __future__ import (absolute_import, division, print_function)
try:
    import pathos.multiprocessing as mp
    PATHOS_FOUND = True
except ImportError:
    PATHOS_FOUND = False

import numpy as np
import six
import os

from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty, mtd
# noinspection PyProtectedMember
from mantid.api._api import WorkspaceGroup
from mantid.simpleapi import CreateWorkspace, CloneWorkspace, GroupWorkspaces, Scale, SetSampleMaterial, Rebin, \
                             SaveAscii, Load, DeleteWorkspace
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty
from AbinsModules import LoadCASTEP, LoadCRYSTAL, CalculateS, AbinsParameters, AbinsConstants, InstrumentProducer


# noinspection PyPep8Naming,PyMethodMayBeStatic
class Abins(PythonAlgorithm):

    _dft_program = None
    _phonon_file = None
    _experimental_file = None
    _temperature = None
    _scale = None
    _sample_form = None
    _instrument_name = None
    _atoms = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    _extracted_dft_data = None

    def category(self):
        return "Simulation"

        # ----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates inelastic neutron scattering."

        # ----------------------------------------------------------------------------------------
    def PyInit(self):

        # Declare all properties
        self.declareProperty(name="DFTprogram",
                             direction=Direction.Input,
                             defaultValue="CASTEP",
                             validator=StringListValidator(["CASTEP", "CRYSTAL"]),
                             doc="DFT program which was used for a phonon calculation.")

        self.declareProperty(FileProperty("PhononFile", "",
                             action=FileAction.Load,
                             direction=Direction.Input,
                             extensions=["phonon", "out"]),
                             doc="File with the data from a phonon calculation.")

        self.declareProperty(FileProperty("ExperimentalFile", "",
                             action=FileAction.OptionalLoad,
                             direction=Direction.Input,
                             extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="Temperature",
                             direction=Direction.Input,
                             defaultValue=10.0,
                             doc="Temperature in K for which dynamical structure factor S should be calculated.")

        self.declareProperty(name="Scale", defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="SampleForm",
                             direction=Direction.Input,
                             defaultValue="Powder",
                             validator=StringListValidator(AbinsConstants.ALL_SAMPLE_FORMS),
                             #doc="Form of the sample: SingleCrystal or Powder.")
                             doc="Form of the sample: Powder.")

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             # validator=StringListValidator(AbinsConstants.ALL_INSTRUMENTS)
                             validator=StringListValidator(["TOSCA"]),
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
                                 "CRYSTAL": self._validate_crystal_input_file}

        issues = dict()

        temperature = self.getProperty("Temperature").value
        if temperature < 0:
            issues["Temperature"] = "Temperature must be positive."

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        dft_program = self.getProperty("DFTprogram").value
        phonon_filename = self.getProperty("PhononFile").value
        output = input_file_validators[dft_program](filename_full_path=phonon_filename)
        if output["Invalid"]:
            issues["PhononFile"] = output["Comment"]

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

        # 2) read DFT data
        dft_loaders = {"CASTEP": LoadCASTEP, "CRYSTAL": LoadCRYSTAL}
        dft_reader = dft_loaders[self._dft_program](input_dft_filename=self._phonon_file)
        dft_data = dft_reader.get_formatted_data()
        prog_reporter.report("Phonon data has been read.")

        # 3) calculate S
        s_calculator = CalculateS(filename=self._phonon_file, temperature=self._temperature,
                                  sample_form=self._sample_form, abins_data=dft_data,
                                  instrument=self._instrument,
                                  quantum_order_num=self._num_quantum_order_events)
        s_data = s_calculator.get_formatted_data()
        prog_reporter.report("Dynamical structure factors have been determined.")

        # 4) get atoms for which S should be plotted
        self._extracted_dft_data = dft_data.get_atoms_data().extract()
        num_atoms = len(self._extracted_dft_data)
        all_atoms_symbols = set([self._extracted_dft_data["atom_%s" % atom]["symbol"] for atom in range(num_atoms)])

        if len(self._atoms) == 0:  # case: all atoms
            atoms_symbol = all_atoms_symbols
        else:  # case selected atoms
            if len(self._atoms) != len(set(self._atoms)):  # only different types
                raise ValueError("Not all user defined atoms are unique.")

            for atom_symbol in self._atoms:
                if atom_symbol not in all_atoms_symbols:
                    raise ValueError("User defined atom not present in the system.")
            atoms_symbol = self._atoms
        prog_reporter.report("Atoms, for which dynamical structure factors should be plotted, have been determined.")

        # at the moment only types of atom, e.g, for  benzene three options -> 1) C, H;  2) C; 3) H
        # 5) create workspaces for atoms in interest
        workspaces = []
        if self._sample_form == "Powder":
            workspaces.extend(self._create_partial_s_per_type_workspaces(atoms_symbols=atoms_symbol, s_data=s_data))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            total_atom_workspaces = []
            for ws in workspaces:
                if "total" in ws:
                    total_atom_workspaces.append(ws)
            total_workspace = self._create_total_workspace(partial_workspaces=total_atom_workspaces)
            workspaces.insert(0, total_workspace)
            prog_reporter.report("Workspace with total S  has been constructed.")

        # 7) add experimental data if available to the collection of workspaces
        if self._experimental_file != "":
            workspaces.insert(0, self._create_experimental_data_workspace().getName())
            prog_reporter.report("Workspace with the experimental data has been constructed.")

        group = ','.join(workspaces)
        GroupWorkspaces(group, OutputWorkspace=self._out_ws_name)

        # 8) save workspaces to ascii_file
        num_workspaces = mtd[self._out_ws_name].getNumberOfEntries()
        for wrk_num in range(num_workspaces):
            wrk = mtd[self._out_ws_name].getItem(wrk_num)
            SaveAscii(InputWorkspace=wrk, Filename=wrk.getName() + ".dat", Separator="Space", WriteSpectrumID=False)
        prog_reporter.report("All workspaces have been saved to ASCII files.")

        # 9) set  OutputWorkspace
        self.setProperty('OutputWorkspace', self._out_ws_name)
        prog_reporter.report("Group workspace with all required  dynamical structure factors has been constructed.")

    def _create_workspaces(self, atoms_symbols=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Creates both partial and total workspaces for all types of atoms.

        @param atoms_symbols: list of atom types for which S should be created
        @param s_data: dynamical factor data of type SData
        @return: workspaces for list of atoms types, S for the particular type of atom
        """
        s_data_extracted = s_data.extract()
        freq = s_data_extracted["frequencies"]
        shape = [self._num_quantum_order_events]

        shape.extend(list(s_data_extracted["atom_0"]["s"]["order_1"].shape))

        s_atom_data = np.zeros(shape=tuple(shape), dtype=AbinsConstants.FLOAT_TYPE)
        shape.pop(0)

        num_atoms = len([key for key in s_data_extracted.keys() if "atom" in key])
        temp_s_atom_data = np.copy(s_atom_data)

        result = []

        for atom_symbol in atoms_symbols:

            # create partial workspaces for the given type of atom
            atom_workspaces = []
            s_atom_data.fill(0.0)

            for atom in range(num_atoms):
                if self._extracted_dft_data["atom_%s" % atom]["symbol"] == atom_symbol:

                    temp_s_atom_data.fill(0.0)

                    for order in range(AbinsConstants.FUNDAMENTALS,
                                       self._num_quantum_order_events + AbinsConstants.S_LAST_INDEX):

                        order_indx = order - AbinsConstants.PYTHON_INDEX_SHIFT
                        temp_s_order = s_data_extracted["atom_%s" % atom]["s"]["order_%s" % order]
                        temp_s_atom_data[order_indx] = temp_s_order

                    s_atom_data += temp_s_atom_data  # sum S over the atoms of the same type

            total_s_atom_data = np.sum(s_atom_data, axis=0)

            atom_workspaces.append(
                self._create_workspace(atom_name=atom_symbol, frequencies=freq, s_points=np.copy(total_s_atom_data),
                                       optional_name="_total"))

            atom_workspaces.append(
                self._create_workspace(atom_name=atom_symbol, frequencies=freq, s_points=np.copy(s_atom_data)))

            result.extend(atom_workspaces)

        return result

    def _create_partial_s_per_type_workspaces(self, atoms_symbols=None, s_data=None):
        """
        Creates workspaces for all types of atoms. Each workspace stores quantum order events for S for the given
        type of atom. It also stores total workspace for the given type of atom.

        @param atoms_symbols: list of atom types for which quantum order events of S  should be calculated
        @param s_data: dynamical factor data of type SData
        @return: workspaces for list of atoms types, each workspace contains  quantum order events of
                 S for the particular atom type
        """

        return self._create_workspaces(atoms_symbols=atoms_symbols, s_data=s_data)

    def _fill_s_workspace(self, freq=None, s_points=None, workspace=None):
        """
        Puts S into workspace(s).
        @param freq:  frequencies
        @param s_points: dynamical factor for the given atom
        @param workspace:  workspace to be filled with S
        """
        if self._nspec == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:
            # only FUNDAMENTALS
            if s_points.shape[0] == AbinsConstants.FUNDAMENTALS:

                CreateWorkspace(DataX=freq,
                                DataY=s_points[0],
                                NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                YUnitLabel="S",
                                OutputWorkspace=workspace,
                                EnableLogging=False)

                # Set correct units on workspace
                self._set_workspace_units(wrk=workspace)

            # total workspaces
            elif len(s_points.shape) == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:

                CreateWorkspace(DataX=freq,
                                DataY=s_points,
                                NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                YUnitLabel="S",
                                OutputWorkspace=workspace,
                                EnableLogging=False)

                # Set correct units on workspace
                self._set_workspace_units(wrk=workspace)

            # quantum order events (FUNDAMENTALS + overtones + combinations for the given order)
            else:

                dim = s_points.shape[0]
                partial_wrk_names = []

                for n in range(dim):
                    seed = "quantum_event_%s" % (n + 1)
                    wrk_name = workspace + "_" + seed
                    partial_wrk_names.append(wrk_name)

                    CreateWorkspace(DataX=freq,
                                    DataY=s_points[n],
                                    NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                    YUnitLabel="S",
                                    OutputWorkspace=wrk_name,
                                    EnableLogging=False)

                    # Set correct units on workspace
                    self._set_workspace_units(wrk=wrk_name)

                group = ','.join(partial_wrk_names)
                GroupWorkspaces(group, OutputWorkspace=workspace)
        # 2D S map
        else:

            n_spec = self._instrument.get_nspec()

            # only FUNDAMENTALS
            if s_points.shape[0] == AbinsConstants.FUNDAMENTALS:

                CreateWorkspace(DataX=freq,
                                DataY=s_points[0].flatten(),
                                NSpec=n_spec,
                                YUnitLabel="S",
                                VerticalAxisValues=self._instrument.get_q_length(),
                                VerticalAxisUnit="MomentumTransfer",
                                OutputWorkspace=workspace,
                                EnableLogging=False)

                # Set correct units on workspace
                self._set_workspace_units(wrk=workspace)

            # total workspaces
            elif s_points.shape[0] == self._instrument.get_nspec():

                CreateWorkspace(DataX=freq,
                                DataY=s_points.flatten(),
                                NSpec=n_spec,
                                YUnitLabel="S",
                                VerticalAxisValues=self._instrument.get_q_length(),
                                VerticalAxisUnit="MomentumTransfer",
                                OutputWorkspace=workspace,
                                EnableLogging=False)

                # Set correct units on workspace
                self._set_workspace_units(wrk=workspace)

            # quantum order events (FUNDAMENTALS + overtones + combinations for the given order)
            else:

                dim = s_points.shape[0]
                partial_wrk_names = []

                for n in range(dim):

                    seed = "quantum_event_%s" % (n + 1)
                    wrk_name = workspace + "_" + seed
                    partial_wrk_names.append(wrk_name)

                    CreateWorkspace(DataX=freq,
                                    DataY=s_points[n].flatten(),
                                    NSpec=n_spec,
                                    YUnitLabel="S",
                                    VerticalAxisValues=self._instrument.get_q_length(),
                                    VerticalAxisUnit="MomentumTransfer",
                                    OutputWorkspace=wrk_name,
                                    EnableLogging=False)

                    # Set correct units on workspace
                    self._set_workspace_units(wrk=wrk_name)

                group = ','.join(partial_wrk_names)
                GroupWorkspaces(group, OutputWorkspace=workspace)

    def _create_total_workspace(self, partial_workspaces=None):

        """
        Sets workspace with total S.
        @param partial_workspaces: list of workspaces which should be summed up to obtain total workspace
        @return: workspace with total S from partial_workspaces
         """
        total_workspace = self._out_ws_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].getNames()
        else:
            local_partial_workspaces = partial_workspaces

        if len(local_partial_workspaces) > 1:

            # get frequencies
            freq = mtd[local_partial_workspaces[0]].dataX(0)

            # initialize S
            if self._nspec == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:
                s_atoms = np.copy(mtd[local_partial_workspaces[0]].dataY(0))
            else:
                s_atoms = np.tile(np.copy(mtd[local_partial_workspaces[0]].dataY(0)), (self._nspec, 1))

            s_atoms.fill(0.0)

            # collect all S
            for partial_ws in local_partial_workspaces:
                if self._nspec == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:
                    s_atoms += mtd[partial_ws].dataY(0)
                else:
                    for i in range(self._nspec):
                        s_atoms[i] += mtd[partial_ws].dataY(i)

            # create workspace with S
            self._fill_s_workspace(freq, s_atoms, total_workspace)

            # rebin
            if self._instrument.get_nspec() == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:

                Rebin(InputWorkspace=total_workspace,
                      Params=[AbinsParameters.min_wavenumber, AbinsParameters.bin_width,
                              AbinsParameters.max_wavenumber],
                      OutputWorkspace=total_workspace)

            else:

                wrk = mtd[total_workspace]
                workspace = self._out_ws_name + "_temp"
                hist_num = wrk.getNumberHistograms()

                for i in range(hist_num):

                    y_data = wrk.dataY(i)

                    CreateWorkspace(DataX=freq,
                                    DataY=y_data,
                                    NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                    OutputWorkspace=workspace,
                                    EnableLogging=False)

                    Rebin(InputWorkspace=workspace,
                          Params=[AbinsParameters.min_wavenumber, AbinsParameters.bin_width,
                                  AbinsParameters.max_wavenumber],
                          OutputWorkspace=workspace)

                DeleteWorkspace(workspace)

        # # Otherwise just repackage the workspace we have as the total
        else:
            CloneWorkspace(InputWorkspace=local_partial_workspaces[0], OutputWorkspace=total_workspace)

        return total_workspace

    def _create_workspace(self, atom_name=None, frequencies=None, s_points=None, optional_name=""):

        """
        Creates workspace for the given frequencies and s_points with S data. After workspace is created it is rebined,
        scaled by cross-section factor and optionally multiplied by the user defined scaling factor.

        @param atom_name: symbol of atom for which workspace should be created
        @param frequencies: frequencies in the form of numpy array for which S(Q, omega) can be plotted
        @param s_points: S(Q, omega)
        @param optional_name: optional part of workspace name
        @return: workspace for the given frequency and S data
        """

        ws_name = self._out_ws_name + "_" + atom_name + optional_name
        self._fill_s_workspace(freq=frequencies, s_points=s_points, workspace=ws_name)

        # 1D S
        if self._instrument.get_nspec() == AbinsConstants.ONE_DIMENTIONAL_SPECTRUM:

            Rebin(InputWorkspace=ws_name,
                  Params=[AbinsParameters.min_wavenumber, AbinsParameters.bin_width, AbinsParameters.max_wavenumber],
                  OutputWorkspace=ws_name)

            wrk = mtd[ws_name]

            if isinstance(wrk, WorkspaceGroup):
                num_wrk = wrk.getNumberOfEntries()
                for n in range(num_wrk):
                    self._scale_workspace(atom_name=atom_name, ws_name=wrk.getItem(n).getName())
            else:
                self._scale_workspace(atom_name=atom_name, ws_name=ws_name)

        # 2D S
        else:

            wrk = mtd[ws_name]
            workspace = self._out_ws_name + "_temp"

            if isinstance(wrk, WorkspaceGroup):

                num_wrk = wrk.getNumberOfEntries()

                x_data = wrk.getItem(0).dataX(0)  # all histograms have the same x-axis

                for n in range(num_wrk):

                    hist_num = wrk.getItem(n).getNumberHistograms()

                    for i in range(hist_num):

                        y_data = wrk.getItem(n).dataY(i)

                        CreateWorkspace(DataX=x_data,
                                        DataY=y_data,
                                        NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                        OutputWorkspace=workspace,
                                        EnableLogging=False)

                        Rebin(InputWorkspace=workspace,
                              Params=[AbinsParameters.min_wavenumber, AbinsParameters.bin_width,
                                      AbinsParameters.max_wavenumber],
                              OutputWorkspace=workspace)

                    self._scale_workspace(atom_name=atom_name, ws_name=wrk.getItem(n).getName())

                DeleteWorkspace(workspace)

            else:

                hist_num = wrk.getNumberHistograms()
                x_data = wrk.dataX(0)

                for i in range(hist_num):

                    y_data = wrk.dataY(i)

                    CreateWorkspace(DataX=x_data,
                                    DataY=y_data,
                                    NSpec=AbinsConstants.ONE_DIMENTIONAL_SPECTRUM,
                                    OutputWorkspace=workspace,
                                    EnableLogging=False)

                    Rebin(InputWorkspace=workspace,
                          Params=[AbinsParameters.min_wavenumber, AbinsParameters.bin_width,
                                  AbinsParameters.max_wavenumber],
                          OutputWorkspace=workspace)

                self._scale_workspace(atom_name=atom_name, ws_name=ws_name)
                DeleteWorkspace(workspace)

        return ws_name

    def _scale_workspace(self, atom_name=None, ws_name=None):
        """
        Performs scaling  workspace by scattering factor and user defined scaling factor.
        """
        # Add the sample material to the workspace
        SetSampleMaterial(InputWorkspace=ws_name, ChemicalFormula=atom_name)

        # Multiply intensity by scattering cross section
        scattering_x_section = None
        if self._scale_by_cross_section == 'Incoherent':
            scattering_x_section = mtd[ws_name].mutableSample().getMaterial().incohScatterXSection()
        elif self._scale_by_cross_section == 'Coherent':
            scattering_x_section = mtd[ws_name].mutableSample().getMaterial().cohScatterXSection()
        elif self._scale_by_cross_section == 'Total':
            scattering_x_section = mtd[ws_name].mutableSample().getMaterial().totalScatterXSection()

        Scale(InputWorkspace=ws_name,
              OutputWorkspace=ws_name,
              Operation='Multiply',
              Factor=scattering_x_section)

        # additional scaling  the workspace if user wants it
        if self._scale != 1:
            Scale(InputWorkspace=ws_name,
                  OutputWorkspace=ws_name,
                  Operation='Multiply',
                  Factor=self._scale)

    def _create_experimental_data_workspace(self):
        """
        Loads experimental data into workspaces.
        @return: workspace with experimental data
        """
        experimental_wrk = Load(self._experimental_file)
        self._set_workspace_units(wrk=experimental_wrk.getName())

        return experimental_wrk

    def _set_workspace_units(self, wrk=None):
        """
        Sets x and y units for a workspace.
        @param wrk: workspace which units should be set
        """

        unitx = mtd[wrk].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Loss", 'cm<sup>-1</sup>')

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
        fwhm = AbinsParameters.fwhm
        if not (isinstance(fwhm, float) and 0.0 < fwhm < 10.0):
            raise RuntimeError("Invalid value of fwhm" + message_end)

        # check delta_width
        delta_width = AbinsParameters.delta_width
        if not (isinstance(delta_width, float) and 0.0 < delta_width < 1.0):
            raise RuntimeError("Invalid value of delta_width" + message_end)

    def _check_tosca_parameters(self, message_end=None):
        """
        Checks TOSCA parameters.
        :param message_end: closing part of the error message.
        """

        # TOSCA final energy in cm^-1
        final_energy = AbinsParameters.tosca_final_neutron_energy
        if not (isinstance(final_energy, float) and final_energy > 0.0):
            raise RuntimeError("Invalid value of final_neutron_energy for TOSCA" + message_end)

        angle = AbinsParameters.tosca_cos_scattering_angle
        if not isinstance(angle, float):
            raise RuntimeError("Invalid value of cosines scattering angle for TOSCA" + message_end)

        resolution_const_a = AbinsParameters.tosca_a
        if not isinstance(resolution_const_a, float):
            raise RuntimeError("Invalid value of constant A for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_b = AbinsParameters.tosca_b
        if not isinstance(resolution_const_b, float):
            raise RuntimeError("Invalid value of constant B for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

        resolution_const_c = AbinsParameters.tosca_c
        if not isinstance(resolution_const_c, float):
            raise RuntimeError("Invalid value of constant C for TOSCA (used by the resolution TOSCA function)" +
                               message_end)

    def _check_folder_names(self, message_end=None):
        """
        Checks folders names.
        :param message_end: closing part of the error message.
        """
        folder_names = []
        dft_group = AbinsParameters.dft_group
        if not isinstance(dft_group, str) or dft_group == "":
            raise RuntimeError("Invalid name for folder in which the DFT data should be stored.")
        folder_names.append(dft_group)

        powder_data_group = AbinsParameters.powder_data_group
        if not isinstance(powder_data_group, str) or powder_data_group == "":
            raise RuntimeError("Invalid value of powder_data_group" + message_end)
        elif powder_data_group in folder_names:
            raise RuntimeError("Name for powder_data_group  already used by as name of another folder.")
        folder_names.append(powder_data_group)

        crystal_data_group = AbinsParameters.crystal_data_group
        if not isinstance(crystal_data_group, str) or crystal_data_group == "":
            raise RuntimeError("Invalid value of crystal_data_group" + message_end)
        elif crystal_data_group in folder_names:
            raise RuntimeError("Name for crystal_data_group already used as a name of another folder.")

        s_data_group = AbinsParameters.s_data_group
        if not isinstance(s_data_group, str) or s_data_group == "":
            raise RuntimeError("Invalid value of s_data_group" + message_end)
        elif s_data_group in folder_names:
            raise RuntimeError("Name for s_data_group already used as a name of another folder.")

    def _check_rebining(self, message_end=None):
        """
        Checks rebinning parameters.
        :param message_end: closing part of the error message.
        """
        pkt_per_peak = AbinsParameters.pkt_per_peak
        if not (isinstance(pkt_per_peak, six.integer_types) and 1 <= pkt_per_peak <= 1000):
            raise RuntimeError("Invalid value of pkt_per_peak" + message_end)

        # bin width is expressed in cm^-1
        bin_width = AbinsParameters.bin_width
        if not (isinstance(bin_width, float) and 0.0 < bin_width <= 10.0):
            raise RuntimeError("Invalid value of bin_width" + message_end)

        min_wavenumber = AbinsParameters.min_wavenumber
        if not (isinstance(min_wavenumber, float) and min_wavenumber >= 0.0):
            raise RuntimeError("Invalid value of min_wavenumber" + message_end)

        max_wavenumber = AbinsParameters.max_wavenumber
        if not (isinstance(max_wavenumber, float) and max_wavenumber > 0.0):
            raise RuntimeError("Invalid number of max_wavenumber" + message_end)

        if min_wavenumber > max_wavenumber:
            raise RuntimeError("Invalid energy window for rebinning.")

    def _check_threshold(self, message_end=None):
        """
        Checks acoustic phonon threshold.
        :param message_end: closing part of the error message.
        """
        acoustic_threshold = AbinsParameters.acoustic_phonon_threshold
        if not (isinstance(acoustic_threshold, float) and acoustic_threshold >= 0.0):
            raise RuntimeError("Invalid value of acoustic_phonon_threshold" + message_end)

        # check s threshold
        s_absolute_threshold = AbinsParameters.s_absolute_threshold
        if not (isinstance(s_absolute_threshold, float) and s_absolute_threshold > 0.0):
            raise RuntimeError("Invalid value of s_absolute_threshold" + message_end)

        s_relative_threshold = AbinsParameters.s_relative_threshold
        if not (isinstance(s_relative_threshold, float) and s_relative_threshold > 0.0):
            raise RuntimeError("Invalid value of s_relative_threshold" + message_end)

    def _check_chunk_size(self, message_end=None):
        """
        Check optimal size of chunk
        :param message_end: closing part of the error message.
        """
        optimal_size = AbinsParameters.optimal_size
        if not (isinstance(optimal_size, six.integer_types) and optimal_size > 0):
            raise RuntimeError("Invalid value of optimal_size" + message_end)

    def _check_threads(self, message_end=None):
        """
        Checks number of threads
        :param message_end: closing part of the error message.
        """
        if PATHOS_FOUND:
            atoms_threads = AbinsParameters.atoms_threads
            if not (isinstance(atoms_threads, (int, long)) and 1 <= atoms_threads <= mp.cpu_count()):
                raise RuntimeError("Invalid number of threads for parallelisation over atoms" + message_end)

            q_threads = AbinsParameters.q_threads
            if not (isinstance(q_threads, (int, long)) and 1 <= q_threads <= mp.cpu_count()):
                raise RuntimeError("Invalid number of threads for parallelisation over q" + message_end)

            if atoms_threads * q_threads > mp.cpu_count():
                raise RuntimeError("User asked for more threads than available.")

    def _validate_crystal_input_file(self, filename_full_path=None):
        """
        Method to validate input file for CRYSTAL DFT program.
        @param filename_full_path: full path of a file to check.
        @return: True if file is valid otherwise false.
        """
        logger.information("Validate CRYSTAL phonon file: ")

        output = {"Invalid": False, "Comment": ""}
        msg_err = "Invalid %s file. " % filename_full_path
        msg_rename = "Please rename your file and try again."

        # check  extension of a file
        filename_ext = os.path.splitext(filename_full_path)[1]
        if filename_ext != ".out":
            return dict(Invalid=True,
                        Comment=msg_err + "Output from DFT program " + self._dft_program + " is expected."
                                        + " The expected extension of file is .out . (found: " + filename_ext + ") "
                                        + msg_rename)
        return output

    def _validate_castep_input_file(self, filename_full_path=None):
        """
        Check if input DFT phonon file has been produced by CASTEP. Currently the crucial keywords in the first few
        lines are checked (to be modified if a better validation is found...)


        :param filename_full_path: full path of a file to check
        :return: Dictionary with two entries "Invalid", "Comment". Valid key can have two values: True/ False. As it
                 comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """
        logger.information("Validate CASTEP phonon file: ")

        output = {"Invalid": False, "Comment": ""}
        msg_err = "Invalid %s file. " % filename_full_path
        msg_rename = "Please rename your file and try again."

        # check  extension of a file
        filename_ext = os.path.splitext(filename_full_path)[1]
        if filename_ext != ".phonon":
            return dict(Invalid=True,
                        Comment=msg_err + "Output from DFT program " + self._dft_program + " is expected."
                                        + " The expected extension of file is .phonon . (found: " + filename_ext + ") "
                                        + msg_rename)

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
        :return: string containing one non empty line
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
        :return:  True is pattern present in the line, otherwise False
        """
        return one_line and pattern in one_line.replace(" ", "")

    def _get_properties(self):
        """
        Loads all properties to object's attributes.
        """

        self._dft_program = self.getProperty("DFTprogram").value
        self._phonon_file = self.getProperty("PhononFile").value
        self._experimental_file = self.getProperty("ExperimentalFile").value
        self._temperature = self.getProperty("Temperature").value
        self._scale = self.getProperty("Scale").value
        self._sample_form = self.getProperty("SampleForm").value

        instrument_name = self.getProperty("Instrument").value
        if instrument_name in AbinsConstants.ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
            instrument_producer = InstrumentProducer()
            self._instrument = instrument_producer.produce_instrument(name=self._instrument_name)
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)
        self._nspec = self._instrument.get_nspec()

        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value

        # conversion from str to int
        self._num_quantum_order_events = int(self.getProperty("QuantumOrderEventsNumber").value)

        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')
        self._calc_partial = (len(self._atoms) > 0)


try:
    AlgorithmFactory.subscribe(Abins)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
