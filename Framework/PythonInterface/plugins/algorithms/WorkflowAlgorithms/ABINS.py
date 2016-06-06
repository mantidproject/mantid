from mantid.kernel import logger, StringListValidator, Direction
from mantid.api import AlgorithmFactory,  FileAction, FileProperty, PythonAlgorithm

class ABINS(PythonAlgorithm):

    _temperature = None
    _phononFile = None
    _experimentalFile = None
    _sampleForm = None
    _Qvectors = None
    _intrinsicBroadening = None
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

        self.declareProperty(name="Temperature",
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


    def validateInputs(self):
        pass

    def PyExec(self):

        self._get_properties()

    def _get_properties(self):

        self._phononFile = self.getProperty("Phonon File").value
        self._experimentalFile = self.getProperty("Experimental File").value
        self._temperature = self.getProperty("Temperature").value
        self._sampleForm = self.getProperty("Sample Form").value
        self._Qvectors = self.getProperty("Q vectors").value
        self._intrinsicBroadening = self.getProperty("Intrinsic Broadening").value
        self._instrumentalBroadening = self.getProperty("Instrumental Broadening").value
        self._structureFactorMode = self.getProperty("Dynamical Structure Factor").value
        self._threadsNumber = self.getProperty("Number of threads")
        self._saveS = self.getProperty("Save S")

try:
    AlgorithmFactory.subscribe(ABINS)
except ImportError:
    logger.debug('Failed to subscribe algorithm SimulatedDensityOfStates; The python package may be missing.')
