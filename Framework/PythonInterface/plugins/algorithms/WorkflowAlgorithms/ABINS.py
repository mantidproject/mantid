import multiprocessing

from mantid.api import AlgorithmFactory,  FileAction, FileProperty, PythonAlgorithm, Progress, WorkspaceProperty
from mantid.kernel import logger, StringListValidator, Direction

from AbinsModules import LoadCASTEP


class ABINS(PythonAlgorithm):

    _temperature = None
    _phononFile = None
    _experimentalFile = None
    _sampleForm = None
    _Qvectors = None
    _intrinsicBroadening = None
    _instrument=None
    _instrumentalBroadening = None
    _structureFactorMode = None
    _structureFactorUnrefined = None
    _structureFactorRefined = None
    _threadsNumber = None
    _saveS = None

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

        self.declareProperty(name="Sample Form",
                             direction=Direction.Input,
                             defaultValue="Powder",
                             validator=StringListValidator(["SingleCrystal", "Powder"]),
                             doc="Form of the sample: SingleCrystal or Powder.")

        self.declareProperty(name="Intrinsic Broadening",
                             direction=Direction.Input,
                             defaultValue="Gaussian",
                             validator=StringListValidator(["None", "Gaussian", "Lorentzian", "Voigt"]),
                             doc="Natural broadening of spectrum. This function will be convoluted with the theoretical spectrum.")

        self.declareProperty(name="Instrument",
                             direction=Direction.Input,
                             defaultValue="TOSCA",
                             validator=StringListValidator(["None", "TOSCA"]),
                             doc="Name of an instrument for which analysis should be performed.")

        self.declareProperty(name="Dynamical Structure Factor",
                             direction=Direction.Input,
                             defaultValue="Full",
                             validator=StringListValidator(["Full", "FundamentalsAndOvertones", "Atoms"]),
                             doc="Theoretical dynamical structure S. The valid options are Full FundamentalsAndOvertones, Atoms")

        self.declareProperty(name="Number of threads",
                             direction=Direction.Input,
                             defaultValue=1,
                             doc="Number of threads for parallel calculations of Debye-Waller factors and dynamical structure factor; Default is 1.")

        self.declareProperty(name="Save S",
                             direction=Direction.Input,
                             defaultValue=False,
                             doc="Save unrefined and refined dynamical structure factor to *hdf5 file.")

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc="Name to give the output workspace.")


    def validateInputs(self):
        """
        Performs input validation.

        Used to ensure the user is requesting a valid mode.
        """
        issues = dict()

        # TODO: check consistency between chosen DFT program and file with DFT phonon data

        temperature = self.getPropertyValue("Temperature")
        if temperature < 0:
            issues["Temperature"]="Temperature must be positive!"

        tot_num_cpu=multiprocessing.cpu_count()
        num_cpu=self.getPropertyValue("Number of threads")
        if num_cpu<1:
            issues["Number of threads"] = "Number of threads cannot be smaller than 1!"
        elif num_cpu>tot_num_cpu:
            issues["Number of threads"] = "Number of threads cannot be larger than available number of threads!"

        return issues


    def PyExec(self):

        steps = 10
        begin = 0
        end = 1.0
        prog_reporter = Progress(self, begin, end, steps)

        self._get_properties()
        prog_reporter.report("Input data from the user has been collected.")

        castep_reader=LoadCASTEP(self._phononFile)
        castep_data=castep_reader.readPhononFile()
        prog_reporter.report("Phonon file has been read.")



    def _get_properties(self):

        self._phononFile = self.getProperty("Phonon File").value
        self._experimentalFile = self.getProperty("Experimental File").value
        self._temperature = self.getProperty("Temperature").value
        self._sampleForm = self.getProperty("Sample Form").value
        self._intrinsicBroadening = self.getProperty("Intrinsic Broadening").value
        self._instrument=self.getProperty("Instrument").value
        self._structureFactorMode = self.getProperty("Dynamical Structure Factor").value
        self._threadsNumber = self.getProperty("Number of threads")
        self._saveS = self.getProperty("Save S")

try:
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
