import multiprocessing
import numpy as np

from mantid.api import AlgorithmFactory, FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty, mtd
from mantid.api._api import WorkspaceGroup
from mantid.simpleapi import CreateWorkspace, CloneWorkspace, GroupWorkspaces, Scale, SetSampleMaterial, \
                             DeleteWorkspace, Rebin, Load, SaveAscii
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty

from AbinsModules import LoadCASTEP, CalculateS, AbinsParameters, AbinsConstants


# noinspection PyPep8Naming
class ABINS(PythonAlgorithm):

    _dft_program = None
    _phononFile = None
    _experimentalFile = None
    _temperature = None
    _scale = None
    _sampleForm = None
    _instrument = None
    _atoms = None
    _sum_contributions = None
    _scale_by_cross_section = None
    _output_workspace_name = None
    _calc_partial = None
    _out_ws_name = None
    _num_quantum_order_events = None
    # ----------------------------------------------------------------------------------------

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
                             extensions=["phonon"]),
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
                             validator=StringListValidator(["SingleCrystal", "Powder"]),
                             doc="Form of the sample: SingleCrystal or Powder.")

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             validator=StringListValidator(["None", "TOSCA"]),
                             doc="Name of an instrument for which analysis should be performed.")

        self.declareProperty(StringArrayProperty("Atoms", Direction.Input),
                             doc="List of atoms to use to calculate partial S."
                                 "If left blank, S for all types of atoms will be calculated")

        self.declareProperty(name="SumContributions", defaultValue=False,
                             doc="Sum the partial dynamical structure factors into a single workspace.")

        self.declareProperty(name="ScaleByCrossSection", defaultValue='Incoherent',
                             validator=StringListValidator(['Total', 'Incoherent', 'Coherent']),
                             doc="Scale the partial dynamical structure factors by the scattering cross section.")

        self.declareProperty(name="QuantumOrderEventsNumber", defaultValue='1',
                             validator=StringListValidator(['1', '2', '3', '4']),
                             doc="Number of quantum order effects included in the calculation "
                                 "(1 -> fundamentals, 2-> first overtone + fundamentals + "
                                 "2nd order combinations, 3-> fundamentals + first overtone + second overtone + 2nd "
                                 "order combinations + 3rd order combinations etc...)")

        self.declareProperty(WorkspaceProperty("OutputWorkspace", '', Direction.Output),
                             doc="Name to give the output workspace.")

    def validateInputs(self):
        """
        Performs input validation. Use to ensure the user has defined a consistent set of parameters.
        """
        issues = dict()

        temperature = self.getProperty("Temperature").value
        if temperature < 0:
            issues["Temperature"] = "Temperature must be positive."

        scale = self.getProperty("Scale").value
        if scale < 0:
            issues["Scale"] = "Scale must be positive."

        dft_program = self.getProperty("DFTprogram").value
        phonon_filename = self.getProperty("PhononFile").value

        if dft_program == "CASTEP":
            output = self._validate_castep_input_file(filename=phonon_filename)
            if output["Invalid"]:
                issues["PhononFile"] = output["Comment"]
        elif dft_program == "CRYSTAL":
            issues["DFTprogram"] = "Support for CRYSTAL DFT program not implemented yet."

        workspace_name = self.getPropertyValue("OutputWorkspace")
        if workspace_name in mtd:
            issues["OutputWorkspace"] = "Workspace with name " + workspace_name + " already in use; please give " \
                                                                                  "a different name for workspace."
        elif workspace_name == "":
            issues["OutputWorkspace"] = "Please specify name of workspace."

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
        dft_data = None

        if self._dft_program == "CASTEP":

            dft_reader = LoadCASTEP(input_DFT_filename=self._phononFile)

        else:

            raise RuntimeError("Currently only output files from CASTEP are supported.")

        dft_data = dft_reader.getData()

        prog_reporter.report("Phonon data has been read.")

        # 3) calculate S
        s_calculator = CalculateS(filename=self._phononFile, temperature=self._temperature,
                                  sample_form=self._sampleForm, abins_data=dft_data, instrument_name=self._instrument,
                                  quantum_order_events_num=self._num_quantum_order_events)

        s_data = s_calculator.getData()

        prog_reporter.report("Dynamical structure factors have been determined.")

        # 4) get atoms for which S should be plotted
        _data = dft_data.getAtomsData().extract()
        all_atoms_symbols = set([atom["symbol"] for atom in _data])

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
        if self._sampleForm == "Powder":
            workspaces.extend(self._create_partial_s_per_type_workspaces(atoms_symbols=atoms_symbol, s_data=s_data))
        prog_reporter.report("Workspaces with partial dynamical structure factors have been constructed.")

        # 6) Create a workspace with sum of all atoms if required
        if self._sum_contributions:
            total_atom_workspaces = []
            for ws in workspaces:
                if "total" in ws:
                    total_atom_workspaces.append(ws)
            total_workspace = self._create_total_workspace(partial_workspaces=total_atom_workspaces,
                                                           workspace_core_name=self._out_ws_name)
            workspaces.insert(0, total_workspace)
            prog_reporter.report("Workspace with total S  has been constructed.")

        # 7) add experimental data if available to the collection of workspaces
        if self._experimentalFile != "":
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
        freq_dic = s_data_extracted["frequencies"]
        max_size = freq_dic["order_%s" % AbinsConstants.fundamentals].size
        items = freq_dic.keys()
        items.remove("order_%s" % AbinsConstants.fundamentals)
        for item in items:
            if max_size < freq_dic[item].size:
                max_size = freq_dic[item].size
        freq = np.zeros((self._num_quantum_order_events, max_size))

        for exponential in range(AbinsConstants.fundamentals,
                                 self._num_quantum_order_events + AbinsConstants.s_last_index):
            exponential_indx = exponential - AbinsConstants.python_index_shift
            temp_freq = freq_dic["order_%s" % exponential]
            freq[exponential_indx][:temp_freq.size] = temp_freq

        s_atom_data = np.copy(freq)
        atoms_data = s_data_extracted["atoms_data"]
        num_atoms = len(atoms_data)
        result_workspaces = []
        temp_s_atom_data = np.copy(s_atom_data)
        for atom_symbol in atoms_symbols:
            # create partial workspaces for the given type of atom
            atom_workspaces = []
            s_atom_data.fill(0.0)
            for atom in range(num_atoms):
                if atoms_data["atom_%s" % atom]["symbol"] == atom_symbol:
                    temp_s_atom_data.fill(0)
                    for order in range(AbinsConstants.fundamentals,
                                       self._num_quantum_order_events + AbinsConstants.s_last_index):
                        order_indx = order - AbinsConstants.python_index_shift
                        temp_order = atoms_data["atom_%s" % atom]["s"]["order_%s" % order]
                        temp_s_atom_data[order_indx, :temp_order.size] = temp_order
                    np.add(s_atom_data, temp_s_atom_data, s_atom_data)  # sum S over the atoms of the same type

            atom_workspaces.append(self._create_workspace(atom_name=atom_symbol,
                                                          frequencies=freq,
                                                          s_points=np.copy(s_atom_data)))

            # create total workspace for the given type of atom
            ws_name = self._out_ws_name + "_" + atom_symbol
            atom_workspaces.insert(0, self._create_total_workspace(partial_workspaces=atom_workspaces,
                                                                   workspace_core_name=ws_name))
            result_workspaces.extend(atom_workspaces)

        return result_workspaces

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

        # only fundamentals
        if s_points.shape[0] == 1:

            temp_freq, temp_s = self._rearrange_freq(freq=freq[0], s_array=s_points[0])
            CreateWorkspace(DataX=temp_freq,
                            DataY=temp_s,
                            NSpec=1,
                            YUnitLabel="S",
                            OutputWorkspace=workspace,
                            EnableLogging=False)
            #
            # Set correct units on workspace
            self._set_workspace_units(wrk=workspace)

        # total workspaces
        elif len(s_points.shape) == 1:

            temp_freq, temp_s = self._rearrange_freq(freq=freq, s_array=s_points)
            CreateWorkspace(DataX=temp_freq,
                            DataY=temp_s,
                            NSpec=1,
                            YUnitLabel="S",
                            OutputWorkspace=workspace,
                            EnableLogging=False)

            # Set correct units on workspace
            self._set_workspace_units(wrk=workspace)

        # quantum order events (fundamentals + overtones + combinations for the given order)
        else:

            dim = s_points.shape[0]
            partial_wrk_names = []

            for n in range(dim):
                seed = "quantum_event_%s" % (n + 1)
                wrk_name = workspace + "_" + seed
                partial_wrk_names.append(wrk_name)
                temp_freq, temp_s = self._rearrange_freq(freq=freq[n], s_array=s_points[n])

                CreateWorkspace(DataX=temp_freq,
                                DataY=temp_s,
                                NSpec=1,
                                YUnitLabel="S",
                                OutputWorkspace=wrk_name,
                                EnableLogging=False)

                # Set correct units on workspace
                self._set_workspace_units(wrk=wrk_name)

            group = ','.join(partial_wrk_names)
            GroupWorkspaces(group, OutputWorkspace=workspace)

    def _create_total_workspace(self, partial_workspaces=None, workspace_core_name=None):

        """
        Sets workspace with total S.
        @param partial_workspaces: list of workspaces which should be summed up to obtain total workspace
        @return: workspace with total S from partial_workspaces
        """

        total_workspace = workspace_core_name + "_total"

        if isinstance(mtd[partial_workspaces[0]], WorkspaceGroup):
            local_partial_workspaces = mtd[partial_workspaces[0]].getNames()
        else:
            local_partial_workspaces = partial_workspaces

        if len(local_partial_workspaces) > 1:

            freq = np.zeros(AbinsConstants.total_workspace_size, dtype=AbinsConstants.float_type)
            s_atoms = np.zeros(AbinsConstants.total_workspace_size, dtype=AbinsConstants.float_type)
            ws_x = mtd[local_partial_workspaces[0]].dataX(0)  # all total workspaces have the same x values
            freq[:ws_x.size] = ws_x
            for partial_ws in local_partial_workspaces:
                ws_y = mtd[partial_ws].dataY(0)
                s_atoms[:ws_y.size] += ws_y

            self._fill_s_workspace(freq, s_atoms, total_workspace)

        # Otherwise just repackage the workspace we have as the total
        else:
            CloneWorkspace(InputWorkspace=local_partial_workspaces[0],
                           OutputWorkspace=total_workspace)
        return total_workspace

    def _create_workspace(self, atom_name=None, frequencies=None, s_points=None):

        """
        Creates workspace for the given frequencies and s_points with S data. After workspace is created it is rebined,
        scaled by cross-section factor and optionally multiplied by the user defined scaling factor.

        @param atom_name: symbol of atom for which workspace should be created
        @param frequencies: frequencies in the form of numpy array for which S(Q, omega) can be plotted
        @param s_points: S(Q, omega)
        @return: workspace for the given frequency and S data

        """

        ws_name = self._out_ws_name + "_" + atom_name
        self._fill_s_workspace(freq=frequencies, s_points=s_points, workspace=ws_name)

        # rebining
        Rebin(InputWorkspace=ws_name,
              Params=[AbinsParameters.min_wavenumber,  AbinsParameters.bin_width, AbinsParameters.max_wavenumber],
              OutputWorkspace=ws_name)

        if s_points.shape[0] == 1:
            self._scale_workspace(atom_name=atom_name, ws_name=ws_name)
        else:
            gr_wrk = mtd[ws_name]
            num_wrk = gr_wrk.getNumberOfEntries()
            for n in range(num_wrk):
                self._scale_workspace(atom_name=atom_name, ws_name=gr_wrk.getItem(n).getName())

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
        experimental_wrk = Load(self._experimentalFile)
        self._set_workspace_units(wrk=experimental_wrk.getName())

        return experimental_wrk

    def _rearrange_freq(self, freq=None, s_array=None):

        # arrays will be modified so make sure we work on their copies
        local_freq = np.copy(freq)
        local_s = np.copy(s_array)

        # sort arrays
        sorted_indx = local_freq.argsort()
        local_freq = local_freq[sorted_indx]
        local_s = local_s[sorted_indx]

        return local_freq, local_s

    def _set_workspace_units(self, wrk=None):
        """
        Sets x and y units for a workspace.
        @param wrk: workspace which units should be set
        """

        unitx = mtd[wrk].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Loss", 'cm^-1')

        mtd[wrk].setYUnitLabel("S /Arbitrary Units")
        mtd[wrk].setYUnit("Arbitrary Units")

    def _validate_crystal_input_file(self, filename=None):
        """
        Method to validate input file for CRYSTAL DFT program.
        @param filename: name of file.
        @return: True if file is valid otherwise false.
        """
        pass

    def _validate_castep_input_file(self, filename=None):
        """
        Check if input DFT phonon file has been produced by CASTEP. Currently the crucial keywords in the first few
        lines are checked (to be modified if a better validation is found...)


        :param filename: name of the file to check
        :return: Dictionary with two entries "Invalid", "Comment". Valid key can have two values: True/ False. As it
                 comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """
        logger.information("Validate CASTEP phonon file: ")

        output = {"Invalid": False, "Comment": ""}
        msg_err = "Invalid %s file. " % filename
        msg_rename = "Please rename your file and try again."

        # check name of file
        if "." not in filename:
            return dict(Invalid=True, Comment=msg_err + " One dot '.' is expected in the name of file! " + msg_rename)

        if filename.count(".") != 1:
            return dict(Invalid=True, Comment=msg_err + " Only one dot should be in the name of file! " + msg_rename)

        if filename[filename.find(".") + 1:].lower() != "phonon":
            return dict(Invalid=True, Comment=msg_err + " The expected extension of file is phonon "
                                                        "(case of letter does not matter)! " + msg_rename)

        # check a structure of the header part of file.
        # Here fortran convention is followed: case of letter does not matter
        with open(filename, "r") as castep_file:

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
        self._phononFile = self.getProperty("PhononFile").value
        self._experimentalFile = self.getProperty("ExperimentalFile").value
        self._temperature = self.getProperty("Temperature").value
        self._scale = self.getProperty("Scale").value
        self._sampleForm = self.getProperty("SampleForm").value
        self._instrument = self.getProperty("Instrument").value
        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value

        # conversion from str to int
        self._num_quantum_order_events = int(self.getProperty("QuantumOrderEventsNumber").value)
        self._evaluate_combinations = self._num_quantum_order_events > AbinsConstants.quantum_order_one
        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')

        self._calc_partial = (len(self._atoms) > 0)

try:
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
