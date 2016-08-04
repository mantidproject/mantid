import multiprocessing
import numpy as np

from mantid.api import AlgorithmFactory,  FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty, mtd
from mantid.simpleapi import  CreateWorkspace, CloneWorkspace, GroupWorkspaces, Scale, RenameWorkspace, SetSampleMaterial, DeleteWorkspace
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty

from AbinsModules import LoadCASTEP, CalculateS, Constants


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

    # ----------------------------------------------------------------------------------------

    def category(self):
        return "Simulation"

        # ----------------------------------------------------------------------------------------

    def summary(self):
        return "Calculates inelastic neutron scattering."

        # ----------------------------------------------------------------------------------------

    def PyInit(self):

        # Declare all properties
        self.declareProperty(name="DFT program",
                             direction=Direction.Input,
                             defaultValue="CASTEP",
                             validator=StringListValidator(["CASTEP", "CRYSTAL"]),
                             doc="DFT program which was used for phonon calculation.")

        self.declareProperty(FileProperty("Phonon File", "",
                             action=FileAction.Load,
                             direction=Direction.Input,
                             extensions=["phonon"]),
                             doc="File with the data from phonon calculation.")

        self.declareProperty(FileProperty("Experimental File", "",
                             action=FileAction.OptionalLoad,
                             direction=Direction.Input,
                             extensions=["raw", "dat"]),
                             doc="File with the experimental inelastic spectrum to compare.")

        self.declareProperty(name="Temperature [K]",
                             direction=Direction.Input,
                             defaultValue=10.0,
                             doc="Temperature in K for which dynamical structure factor S should be calculated.")

        self.declareProperty(name='Scale', defaultValue=1.0,
                             doc='Scale the intensity by the given factor. Default is no scaling.')

        self.declareProperty(name="Sample Form",
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
                             doc="List of atoms to use to calculate partial S." \
                                 "If left blank, total S will be calculated")

        self.declareProperty(name='SumContributions', defaultValue=False,
                             doc="Sum the partial dynamical structure factors into a single workspace.")

        self.declareProperty(name='ScaleByCrossSection', defaultValue='Incoherent',
                             validator=StringListValidator(['Total', 'Incoherent', 'Coherent']),
                             doc="Sum the partial dynamical structure factor by the scattering cross section.")


        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace.")


    def validateInputs(self):
        """
        Performs input validation.

        Used to ensure the user is requesting a valid mode.
        """
        issues = dict()

        temperature = self.getPropertyValue("Temperature [K]")
        if temperature < 0:
            issues["Temperature"] = "Temperature must be positive!"

        dft_filename = self.getProperty("DFT program")
        if dft_filename == "CASTEP":
            output = self._validate_castep_input_file(filename=dft_filename)
            if not output["Valid"]:
                issues["DFT program"] = output["Comment"]
        elif dft_filename == "CRYSTAL":
            issues["DFT program"] = "Support for CRYSTAL DFT program not implemented yet!"

        sum_contributions = self.getProperty('SumContributions').value
        atoms = self.getProperty('Atoms').value
        calc_partial = len(atoms) > 0
        if not calc_partial and sum_contributions:
            issues['SumContributions'] = 'Cannot sum contributions when not calculating partial density of states'


        return issues


    def PyExec(self):

        steps = 4
        begin = 0
        end = 1.0
        prog_reporter = Progress(self, begin, end, steps)

        # 1) get input parameters from a user
        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        # 2) read dft data
        dft_data = None
        if self._dft_program == "CASTEP":
            dft_reader = LoadCASTEP(input_DFT_filename=self._phononFile)
            dft_data = dft_reader.readPhononFile()
        else:
            raise RuntimeError("Currently only output files from CASTEP are supported.")
        prog_reporter.report("Phonon file has been read.")

        # 3) calculate S
        s_calculator = CalculateS(filename=self._phononFile, temperature=self._temperature,
                                  instrument_name=self._instrument, abins_data=dft_data,
                                  sample_form=self._sampleForm)
        s_data = s_calculator.getS()
        prog_reporter.report("Dynamical structure factor is ready to be plotted.")

        # 4) get atoms for which S should be plotted
        _data  = dft_data.getAtomsData().extract()
        all_atoms_symbols = [atom["symbol"] for atom in _data]

        if len(self._atoms) == 0: # case: all atoms
            atoms_symbol = all_atoms_symbols
        else: # case selected atoms
            if len(self._atoms) != len(set(self._atoms)): # only different types
                raise ValueError("Not all user defined atoms are unique.")

            for atom_symbol in self._atoms:
                if not atom_symbol in all_atoms_symbols:
                    raise ValueError("User defined atom not present in the system.")
            atoms_symbol = self._atoms

        # at the moment only types of atom, e.g, for  benzene three options -> 1) C, H;  2) C; 3) H
        # 5) create workspaces for atoms in interest
        partial_workspaces = self._create_partial_s_workspaces(atoms_symbol=atoms_symbol, s_data=s_data)

        # 6) Create an output workspace:
        #           If  a user wants partial workspaces create an output workspace with all workspaces
        #           If  a user wants total S from the selected atoms then create a workspace with total S and delete partial workspaces
        if self._sum_contributions:

            total_workspace = self._set_total_workspace(partial_workspaces=partial_workspaces)
            # Discard the partial workspaces
            for partial_ws in partial_workspaces:
                DeleteWorkspace(partial_ws)

            # Rename the summed workspace, this will be the output
            #RenameWorkspace(InputWorkspace=sum_workspace, OutputWorkspace=self._out_ws_name)
            logger.debug('Summed workspace: ' + str(total_workspace))

        else:

            group = ','.join(partial_workspaces)
            GroupWorkspaces(group, OutputWorkspace=self._out_ws_name)

            logger.debug('Partial workspaces: ' + str(group))

        self.setProperty('OutputWorkspace', self._out_ws_name)

    def _compute_workspace_for_s(self):
        """
        Puts calculated S into Mantid Workspace so that it can be easily visualise by Mantid utilities.
        @return:
        """

        partial_workspaces, sum_workspace = self._produce_workspace_for_partial_S_workflow()

    def _create_partial_s_workspaces(self, atoms_symbol=None,s_data=None):

        s_data_extracted = s_data.extract()
        freq = s_data_extracted["convoluted_frequencies"]
        dim = freq.shape[0]
        s_atom_data = np.zeros(dim, dtype=Constants.float_type)
        s_all_atoms  = s_data_extracted["atoms_data"]
        num_atoms = len(s_all_atoms)

        partial_workspaces = []
        total_workspace = None

        for atom_symbol in atoms_symbol:
            s_atom_data.fill(0.0)
            for num_atom in range(num_atoms):
                if s_all_atoms[num_atom]["symbol"] == atom_symbol:
                    np.add(s_atom_data, s_all_atoms[num_atom]["value"][:,Constants.overtones_num], s_atom_data) # we sum total S over the atoms of the same type

            # total S for the given atom
            partial_workspaces.append(self._set_workspace(atom_name=atom_symbol, frequencies=freq, s_points=s_atom_data))


        return partial_workspaces


    def _create_s_workspace(self, freq=None, s_points=None, workspace=None):
        """
        Puts S for the given atom into workspace.
        @param freq:  frequencies
        @param s_points: dynamical factor for the given atom
        @param workspace:  workspace to be filled with S
        """

        # order x and y values before saving to workspace
        sorted_indices = freq.argsort()
        freq = freq[sorted_indices]
        s_points = s_points[sorted_indices]

        CreateWorkspace(DataX=freq,
                        DataY=s_points,
                        NSpec=1,
                        VerticalAxisUnit="Text",
                        VerticalAxisValues="S",
                        OutputWorkspace=workspace,
                        EnableLogging=False)

        unitx = mtd[workspace].getAxis(0).setUnit("Label")
        unitx.setLabel("Energy Shift", 'cm^-1')


    def _set_total_workspace(self, partial_workspaces=None):

        total_workspace = self._out_ws_name + "_Total"
        # If there is more than one partial workspace need to sum first spectrum of all
        if len(partial_workspaces) > 1:
            _freq = mtd[partial_workspaces[0]].dataX(0)
            _s_atoms = np.zeros_like(mtd[partial_workspaces[0]].dataY(0))

            for partial_ws in partial_workspaces:
                _s_atoms += mtd[partial_ws].dataY(0)

            self._create_s_workspace(_freq, _s_atoms, total_workspace)

            # Set correct units on total workspace
            mtd[total_workspace].setYUnitLabel('Intensity')

        # Otherwise just repackage the WS we have as the total
        else:
            CloneWorkspace(InputWorkspace=partial_workspaces[0],
                           OutputWorkspace=total_workspace)

        return total_workspace


    def _set_workspace(self, atom_name=None, frequencies=None, s_points=None):

        _ws_name = self._out_ws_name + atom_name

        self._create_s_workspace(freq=frequencies, s_points=s_points, workspace=_ws_name)

        # Set correct units on partial workspace

        mtd[_ws_name].setYUnitLabel('Intensity')

        # Add the sample material to the workspace
        SetSampleMaterial(InputWorkspace=_ws_name,
                          ChemicalFormula=atom_name)

        # Multiply intensity by scattering cross section
        scattering_x_section = None
        if self._scale_by_cross_section == 'Incoherent':
            scattering_x_section = mtd[_ws_name].mutableSample().getMaterial().incohScatterXSection()
        elif self._scale_by_cross_section == 'Coherent':
            scattering_x_section = mtd[_ws_name].mutableSample().getMaterial().cohScatterXSection()
        elif self._scale_by_cross_section == 'Total':
            scattering_x_section = mtd[_ws_name].mutableSample().getMaterial().totalScatterXSection()

        Scale(InputWorkspace=_ws_name,
              OutputWorkspace=_ws_name,
              Operation='Multiply',
              Factor=scattering_x_section)

        #RenameWorkspace(self._out_ws_name,
        #                OutputWorkspace=_ws_name)

        return _ws_name


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
        :return: Dictionary with two entries "Valid", "Comment". Valid key can have two values: True/ False. As it
                 comes to "Comment" it is an empty string if Valid:True, otherwise stores description of the problem.
        """
        logger.debug('Validate CASTEP phonon file: ' + str(partial_ions))

        output = {"Valid": True, "Comment": ""}
        msg_err = "Invalid %s file. " %filename
        msg_rename = "Please rename your file and try again."


        # check name of file
        if "." not in filename:
            output = {"Valid": False, "Comment": msg_err+" One dot '.' is expected in the name of file! " + msg_rename}
            return output
        elif filename.count(".") != 1:
            output = {"Valid": False, "Comment": msg_err+" Only one dot should be in the name of file! " + msg_rename}
            return output
        elif filename[filename.find(".")].lower() != "phonon":
            output = {"Valid": False, "Comment": msg_err+" The expected extension of file is phonon "
                                                       "(case of letter does not matter)! " + msg_rename}
            return output

        # check a structure of the header part of file.
        # Here fortran convention is followed: case of letter does not matter
        with open(filename, "r") as castep_file:

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(line, "beginheader"): # first line is BEGIN header
                output = {"Valid": False, "Comment": msg_err+"The first line should be 'BEGIN header'."}
                return output

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofions"):

                output = {"Valid": False, "Comment": msg_err+"The second line should include 'Number of ions'."}
                return output

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofbranches"):

                output = {"Valid": False, "Comment": msg_err+"The third line should include 'Number of branches'."}
                return output

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line, pattern="numberofwavevectors"):

                output = {"Valid": False, "Comment": msg_err+"The fourth line should include 'Number of wavevectors'."}

            line = self._get_one_line(castep_file)
            if not self._compare_one_line(one_line=line,
                                          pattern="frequenciesin"):

                output = {"Valid": False, "Comment": msg_err+"The fifth line should be 'Frequencies in'."}

        return output


    def _get_one_line(self, file_obj=None):
        """

        :param file_obj:  file object from which reading is done
        :return: string containing one non empty line
        """
        line = file_obj.readline().strip()

        while line and line == "":
            line = file_obj.readline().strip().lower()

        return line


    def _compare_one_line(self, one_line, pattern):
        """

        :param one_line:  line in the for mof string to be compared
        :param pattern: string which should be present in the line after removing white spaces and setting all
                        letters to lower case
        :return:  True is pattern present in the line, otherwise False
        """
        return one_line and pattern in one_line.replace(" ", "")

    def _get_properties(self):

        self._dft_program = self.getProperty("DFT program").value
        self._phononFile = self.getProperty("Phonon File").value
        self._experimentalFile = self.getProperty("Experimental File").value
        self._temperature = self.getProperty("Temperature [K]").value
        self._scale =  self.getProperty("Scale").value
        self._sampleForm = self.getProperty("Sample Form").value
        self._instrument = self.getProperty("Instrument").value
        self._atoms = self.getProperty("Atoms").value
        self._sum_contributions = self.getProperty("SumContributions").value
        self._scale_by_cross_section = self.getPropertyValue('ScaleByCrossSection')
        self._out_ws_name = self.getPropertyValue('OutputWorkspace')
        self._calc_partial = (len(self._atoms) > 0)

try:
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
