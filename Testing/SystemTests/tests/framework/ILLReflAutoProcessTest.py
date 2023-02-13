# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import ReflectometryILLAutoProcess, config, mtd


class D17Cycle192IncoherentSanTest(systemtesting.MantidSystemTest):
    """Tests with VoS11 sample at 2 angles with the data from cycle #192. Uses incoherent summation with sample angle
    option."""

    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config["logging.loggers.root.level"] = "Warning"
        config.appendDataSearchSubDir("ILL/D17/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["VoS11", "D17_VoS11.nxs"]

    def runTest(self):
        name = "VoS11"
        directBeams = "541838,541839"
        reflectedBeams = "541882,541883"
        foregroundWidth = [3, 3]
        angleOffset = [2, 5]
        angleWidth = 5
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType="Incoherent",
            AngleOption="SampleAngle",
            DeltaQFractionBinning=0.5,
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=[3.0, 3.0],
            WavelengthUpperBound=[27.0, 25.0],
            GlobalScaleFactor=0.13,
        )


class D17Cycle192CoherentDanTest(systemtesting.MantidSystemTest):
    """Tests with SiO2 sample at 2 angles with the data from cycle #192. Uses coherent summation with detector angle
    option."""

    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config["logging.loggers.root.level"] = "Warning"
        config.appendDataSearchSubDir("ILL/D17/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["SiO2", "D17_SiO2.nxs"]

    def runTest(self):
        name = "SiO2"
        directBeams = "541838,541839"
        reflectedBeams = "541853,541854"
        foregroundWidth = [3, 3]
        angleOffset = [2, 5]
        angleWidth = 5
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType="Coherent",
            AngleOption="DetectorAngle",
            DeltaQFractionBinning=0.5,
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=[3.0, 3.0],
            WavelengthUpperBound=[27.0, 25.0],
            GlobalScaleFactor=0.13,
        )


class D17Cycle181RoundRobinTest(systemtesting.MantidSystemTest):
    """Tests with RoundRobin sample at 3 angles with the data from cycle #181. Uses incoherent summation with sample
    angle option."""

    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config["logging.loggers.root.level"] = "Warning"
        config.appendDataSearchSubDir("ILL/D17/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["Thick_HR_5", "D17_Thick_HR_5.nxs"]

    def runTest(self):
        name = "Thick_HR_5"
        directBeams = "397812,397806,397808"
        reflectedBeams = "397826+397827,397828,397829+397830+397831+397832"
        foregroundWidth = [4, 5, 8]
        wavelengthLower = [3.0, 1.6, 2.0]
        wavelengthUpper = [27.0, 25.0, 25.0]
        angleOffset = 2
        angleWidth = 10
        ReflectometryILLAutoProcess(
            Run=reflectedBeams,
            DirectRun=directBeams,
            OutputWorkspace=name,
            SummationType="Incoherent",
            AngleOption="SampleAngle",
            DirectLowAngleFrgHalfWidth=foregroundWidth,
            DirectHighAngleFrgHalfWidth=foregroundWidth,
            DirectLowAngleBkgOffset=angleOffset,
            DirectLowAngleBkgWidth=angleWidth,
            DirectHighAngleBkgOffset=angleOffset,
            DirectHighAngleBkgWidth=angleWidth,
            ReflLowAngleFrgHalfWidth=foregroundWidth,
            ReflHighAngleFrgHalfWidth=foregroundWidth,
            ReflLowAngleBkgOffset=angleOffset,
            ReflLowAngleBkgWidth=angleWidth,
            ReflHighAngleBkgOffset=angleOffset,
            ReflHighAngleBkgWidth=angleWidth,
            WavelengthLowerBound=wavelengthLower,
            WavelengthUpperBound=wavelengthUpper,
            DeltaQFractionBinning=0.5,
        )


class D17Cycle213QuartzUserAngle(systemtesting.MantidSystemTest):
    """Tests with quartz sample at 4 angles with the data from cycle 213. Uses incoherent summation with user angle
    option."""

    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "D17"
        config["logging.loggers.root.level"] = "Warning"
        config.appendDataSearchSubDir("ILL/D17/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-6
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["D17_Quartz_213", "D17_Quartz_213.nxs"]

    def runTest(self):
        name = "D17_Quartz_213"
        direct_beams = ",".join(["676978"] * 4)
        reflected_beams = "676979,676980,676981+676982,676983+676985"
        wavelength_lower = [3.7, 3, 3, 3]
        wavelength_upper = 24
        user_angles = [0.406, 0.806, 1.609, 3.207]
        ReflectometryILLAutoProcess(
            Run=reflected_beams,
            DirectRun=direct_beams,
            OutputWorkspace=name,
            SummationType="Incoherent",
            WavelengthLowerBound=wavelength_lower,
            WavelengthUpperBound=wavelength_upper,
            DeltaQFractionBinning=0.5,
            AngleOption="UserAngle",
            Theta=user_angles,
            DirectLowAngleFrgHalfWidth=[3, 3, 3, 3],
            DirectHighAngleFrgHalfWidth=[3, 5, 7, 9],
            ReflLowAngleFrgHalfWidth=[3, 5, 7, 9],
            ReflHighAngleFrgHalfWidth=[3, 5, 7, 9],
            DirectLowAngleBkgOffset=5,
            DirectLowAngleBkgWidth=10,
            DirectHighAngleBkgOffset=5,
            DirectHighAngleBkgWidth=10,
            ReflLowAngleBkgOffset=5,
            ReflLowAngleBkgWidth=10,
            ReflHighAngleBkgOffset=5,
            ReflHighAngleBkgWidth=10,
        )


class FigaroCycle212GravityRefUp(systemtesting.MantidSystemTest):
    """Tests a reduction of C19H16O4 sample at 2 angles with the data from cycle 212. Uses incoherent summation
    with detector angle option, and gravity correction with a reflection reference up."""

    @classmethod
    def setUp(cls):
        cls._original_facility = config["default.facility"]
        cls._original_instrument = config["default.instrument"]
        cls._data_search_dirs = config.getDataSearchDirs()
        config["default.facility"] = "ILL"
        config["default.instrument"] = "FIGARO"
        config["logging.loggers.root.level"] = "Warning"
        config.appendDataSearchSubDir("ILL/FIGARO/")

    @classmethod
    def tearDown(cls):
        config["default.facility"] = cls._original_facility
        config["default.instrument"] = cls._original_instrument
        config.setDataSearchDirs(cls._data_search_dirs)

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-4
        self.tolerance_is_rel_err = True
        self.disableChecking = ["Instrument", "Sample"]
        return ["Cell_d2O", "Figaro_Cell_d2O.nxs"]

    def runTest(self):
        name = "Cell_d2O"
        runs = "743465,743466"
        direct_run = "732252+732254,732255"
        ReflectometryILLAutoProcess(
            Run=runs,
            DirectRun=direct_run,
            OutputWorkspace=name,
            AngleOption="DetectorAngle",
            SummationType="Incoherent",
            WavelengthLowerBound=[3.5, 2.5],
            WavelengthUpperBound=[20, 19],
            DeltaQFractionBinning=0.5,
            GlobalScaleFactor=0.0484221,
            Cleanup="Cleanup ON",
            ReflFitStartWorkspaceIndex=25,
            ReflFitEndWorkspaceIndex=230,
            DirectFitStartWorkspaceIndex=25,
            DirectFitEndWorkspaceIndex=230,
            DirectLowAngleFrgHalfWidth=[3, 7],
            DirectHighAngleFrgHalfWidth=[3, 7],
            ReflLowAngleFrgHalfWidth=[3, 7],
            ReflHighAngleFrgHalfWidth=[3, 7],
            DirectLowAngleBkgOffset=5,
            DirectLowAngleBkgWidth=5,
            DirectHighAngleBkgOffset=5,
            DirectHighAngleBkgWidth=5,
            ReflLowAngleBkgOffset=5,
            ReflLowAngleBkgWidth=5,
            ReflHighAngleBkgOffset=5,
            ReflHighAngleBkgWidth=5,
            ReflFlatBackground="Background Average",
            DirectFlatBackground="Background Average",
            CorrectGravity=True,
        )
