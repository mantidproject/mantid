# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC, abstractmethod
from tempfile import TemporaryDirectory
from types import MappingProxyType

import systemtesting
from mantid.simpleapi import Abins, Abins2D, mtd, DeleteWorkspace


class AbinsTestingMixin(ABC):
    _extensions = {"CASTEP": "phonon", "CRYSTAL": "out", "DMOL3": "outmol", "GAUSSIAN": "log"}

    @property
    @abstractmethod
    def system_name(self) -> str: ...

    @property
    @abstractmethod
    def ab_initio_program(self) -> str: ...

    @property
    def ext(self) -> str:
        return self._extensions[self.ab_initio_program]

    def __init__(self):
        super().__init__()  # Init the base class as well as this mix-in

        # Tolerance used when calling validate()
        self.tolerance = 1e-2

        # This is usually the ref result, can always override later if needed
        self.ref_result = self.system_name + ".nxs"

        # Temporary cache directory, allowing re-use within this MantidSystemTest instance
        self._cache_directory = TemporaryDirectory()

        # Define defaults as an immutable dict to be copied and modified elsewhere
        self.default_kwargs = MappingProxyType(
            {
                "AbInitioProgram": self.ab_initio_program,
                "VibrationalOrPhononFile": f"{self.system_name}.{self.ext}",
                "TemperatureInKelvin": 10,
                "SampleForm": "Powder",
                "Instrument": "TOSCA",
                "BinWidthInWavenumber": 1.0,
                "Atoms": "",
                "SumContributions": True,
                "QuantumOrderEventsNumber": "1",
                "Scale": 1.0,
                "ScaleByCrossSection": "Incoherent",
                "OutputWorkspace": self.system_name,
                "CacheDirectory": self._cache_directory.name,
            }
        )

    def __del__(self):
        """
        Destructor removes output files after tests and workspaces.
        :return:
        """
        self._cache_directory.cleanup()
        mtd.clear()

    def validate(self):
        return self.system_name, self.ref_result


class AbinsCRYSTALTestScratch(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from scratch with input data from CRYSTAL and for 1-2 quantum
    order events is correct.
    """

    system_name = "TolueneScratchAbins"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        Abins(**(self.default_kwargs) | {"QuantumOrderEventsNumber": "2"})

    def excludeInPullRequests(self):
        return True


class AbinsCRYSTALTestBiggerSystem(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from scratch with input data from CRYSTAL and for only 1 quantum
    order event is correct.
    """

    system_name = "Crystalb3lypScratchAbins"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        Abins(**self.default_kwargs)


class AbinsCRYSTALTestT(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this scenario a restart is considered in which data for other
    temperature already exists in an hdf file.

    First a simulation is run for T=10K; then a T=20K for which S has to be
    recalculated. After that another T=10K simulation is requested; the other
    parameters are identical across calculations. In the third run the required
    data /should/ be read from the hdf file. We only check that here that the
    results are correct.

    """

    system_name = "TolueneTAbins"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        case_1_kwargs = self.default_kwargs | {"OutputWorkspace": f"{self.system_name}init", "QuantumOrderEventsNumber": "2"}
        case_2_kwargs = case_1_kwargs | {
            "TemperatureInKelvin": self.default_kwargs["TemperatureInKelvin"] + 10,
            "OutputWorkspace": f"{self.system_name}_mod",
        }
        case_3_kwargs = case_1_kwargs | {"OutputWorkspace": f"{self.system_name}"}

        for case_kwargs in (case_1_kwargs, case_2_kwargs, case_3_kwargs):
            Abins(**case_kwargs)

    def excludeInPullRequests(self):
        return True

    def validate(self):
        """Loosen validation parameters for this test case"""
        self.tolerance = 1e-1
        return super().validate()


class AbinsCRYSTALTestLargerOrder(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from restart with input data from CRYSTAL is correct. Requested order
    of quantum event is larger than the one which is saved to an hdf file so S has to be calculated.
    """

    system_name = "TolueneLargerOrderAbins"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        Abins(**self.default_kwargs)
        DeleteWorkspace(self.system_name)
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2"}))

    def excludeInPullRequests(self):
        return True

    def validate(self):
        """Loosen validation parameters for this test case"""
        self.tolerance = 1e-1
        return super().validate()


class AbinsCRYSTALTestSmallerOrder(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from restart with input data from CRYSTAL is correct. Requested
    order of quantum event is smaller than the one which is saved to an hdf file so S is loaded from an hdf file.
    """

    system_name = "TolueneSmallerOrderAbins"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2"}))
        DeleteWorkspace(self.system_name)
        Abins(**self.default_kwargs)

    def validate(self):
        """Loosen validation parameters for this test case"""
        self.tolerance = 1e-1
        return super().validate()


class AbinsCRYSTALTestScale(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if scaling is correct.
    """

    system_name = "TolueneScale"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2", "Scale": 2.0}))

    def validate(self):
        """Loosen validation parameters for this test case"""
        self.tolerance = 1e-1
        return super().validate()


# noinspection PyAttributeOutsideInit,PyPep8Naming
class AbinsCASTEPNoH(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation for systems without H is correct.
    """

    system_name = "Na2SiF6_CASTEP"
    ab_initio_program = "CASTEP"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2", "ScaleByCrossSection": "Total"}))


# noinspection PyAttributeOutsideInit,PyPep8Naming
class AbinsCASTEP1DDispersion(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation of S from phonon dispersion is correct (1D case).
    """

    system_name = "Mapi"
    ab_initio_program = "CASTEP"

    def runTest(self):
        Abins(**self.default_kwargs)


class AbinsDMOL3TestScratch(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from scratch with input data from DMOL3 and for 1-2 quantum
    order events is correct.
    """

    system_name = "Na2SiF6_DMOL3"
    ab_initio_program = "DMOL3"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2", "ScaleByCrossSection": "Total"}))

    def excludeInPullRequests(self):
        return True


class AbinsGAUSSIANestScratch(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation from scratch with input data from GAUSSIAN and for 1-4 quantum
    order events is correct.
    """

    system_name = "C6H5Cl-Gaussian"
    ab_initio_program = "GAUSSIAN"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2"}))

    def excludeInPullRequests(self):
        return True


class AbinsBinWidth(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation with bin width different than the default value is correct.
    Calculation performed for crystalline benzene for 1st and 2nd quantum event for output from CASTEP and bin width
    3 cm^-1. This system test should be fast so no need for excludeInPullRequests flag.
    """

    system_name = "BenzeneBinWidthCASTEP"
    ab_initio_program = "CASTEP"

    def runTest(self):
        Abins(**(self.default_kwargs | {"QuantumOrderEventsNumber": "2", "BinWidthInWavenumber": 3.0}))


class AbinsCASTEPIsotopes(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    In this benchmark it is tested if calculation of the system with isotopic substitutions: H -> 2H, Li -> 7Li,
    produces correct results. Input data is generated by CASTEP. This system test should be fast so no need for
    excludeInPullRequests flag.
    """

    system_name = "LiOH_H2O_2D2O_CASTEP"
    ab_initio_program = "CASTEP"

    def runTest(self):
        Abins(**(self.default_kwargs | {"BinWidthInWavenumber": 2.0}))


class AbinsCRYSTAL2D(AbinsTestingMixin, systemtesting.MantidSystemTest):
    """
    Check Abins2D runs successfully for a typical use case
    """

    system_name = "AbinsCRYSTAL2D"
    ab_initio_program = "CRYSTAL"

    def runTest(self):
        params_2d = {"Chopper": "A", "ChopperFrequency": str(300), "IncidentEnergy": str(200), "EnergyUnits": "meV"}

        Abins2D(
            AbInitioProgram="CRYSTAL",
            VibrationalOrPhononFile=f"TolueneScratchAbins.{self.ext}",
            TemperatureInKelvin=10,
            Instrument="MARI",
            Atoms="",
            SumContributions=True,
            QuantumOrderEventsNumber="2",
            autoconvolution=True,
            ScaleByCrossSection="Incoherent",
            OutputWorkspace=self.system_name,
            CacheDirectory=self._cache_directory.name,
            **params_2d,
        )

    def validate(self):
        """Tweak validation parameters for this test case"""
        self.tolerance = 1e-4
        self.nanEqual = True
        super().validate()
