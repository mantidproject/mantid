import multiprocessing

from mantid.api import AlgorithmFactory,  FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty
from mantid.kernel import logger, StringListValidator, Direction, StringArrayProperty

from AbinsModules import LoadCASTEP, CalculateS


class ABINS(PythonAlgorithm):

    _temperature = None
    _phononFile = None
    _experimentalFile = None
    _sampleForm = None
    _intrinsicBroadening = None
    _instrument =None
    _structureFactorMode = None
    _output_workspace_name = None
    _scale = None

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

        self.declareProperty(StringArrayProperty('Ions', Direction.Input),
                             doc="List of Ions to use to calculate partial density of states." \
                                 "If left blank, total density of states will be calculated")

        self.declareProperty(name='SumContributions', defaultValue=False,
                             doc="Sum the partial dynamical structure factors into a single workspace.")

        self.declareProperty(name='ScaleByCrossSection', defaultValue='None',
                             validator=StringListValidator(['None', 'Total', 'Incoherent', 'Coherent']),
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
        ions = self.getProperty('Ions').value
        calc_partial = len(ions) > 0
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
        S_data = s_calculator.getS()
        prog_reporter.report("Dynamical structure factor is ready to be plotted.")

        # 4) put S to workspace and plot it

        self.setProperty('OutputWorkspace', self._out_ws_name)


    def _produce_workspace_for_S(self):
        """
        Puts calculated S into Mantid Workspace so that it can be easily visualise by Mantid utilities.
        @return:
        """
        pass


    def _produce_workspace_for_partial_S(self):
        """
        Puts S for the given atom into workspace.
        @return:
        """
        pass


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
        self._sampleForm = self.getProperty("Sample Form").value
        self._instrument = self.getProperty("Instrument").value
        self._ions = self.getProperty("Ions").value
        self._sum_contributions = self.getProperty("SumContributions").value
        self._scale_by_cross_section = self.getProperty("ScaleByCrossSection").value
        self._output_workspace_name = self.getProperty("OutputWorkspace")
        self._calc_partial = (len(self._ions) > 0)

try:
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
