# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import Abins, Abins2D, mtd, DeleteWorkspace

import abins
from abins.constants import (
    ALL_INSTRUMENTS,
    ALL_SUPPORTED_AB_INITIO_PROGRAMS,
    QUANTUM_ORDER_ONE,
    QUANTUM_ORDER_TWO,
    QUANTUM_ORDER_THREE,
    QUANTUM_ORDER_FOUR,
)


class HelperTestingClass(object):
    def __init__(self):
        self._temperature = 10  # K
        self._sample_form = "Powder"
        self._instrument_name = "TOSCA"
        self._atoms = ""
        self._sum_contributions = True
        self._cross_section_factor = "Incoherent"
        self._extension = {"CASTEP": ".phonon", "CRYSTAL": ".out", "DMOL3": ".outmol", "GAUSSIAN": ".log"}
        self._output_name = "output_workspace"
        self._ref = "reference_workspace"
        self._scale = 1.0
        self._bin_width = 1.0

        self._ab_initio_program = None
        self._quantum_order_event = None
        self._system_name = None

    def set_bin_width(self, width):
        if not (isinstance(width, float) and 1.0 <= width <= 10.0):
            raise ValueError("Invalid bin width: {}. ".format(width) + "Valid range is [1.0, 10.0] cm^-1")
        self._bin_width = width

    def set_instrument_name(self, instrument_name=None):
        if instrument_name in ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Wrong instrument.")

    def set_scale(self, scale=None):
        if isinstance(scale, float) and scale > 0.0:
            self._scale = scale
        else:
            raise ValueError("Wrong scale.")

    def set_ab_initio_program(self, ab_initio_program=None):
        if ab_initio_program in ALL_SUPPORTED_AB_INITIO_PROGRAMS:
            self._ab_initio_program = ab_initio_program
        else:
            raise RuntimeError("Unsupported ab initio program: %s " % ab_initio_program)

    def set_order(self, order=None):
        orders = [QUANTUM_ORDER_ONE, QUANTUM_ORDER_TWO, QUANTUM_ORDER_THREE, QUANTUM_ORDER_FOUR]

        if order in orders:
            self._quantum_order_event = order
        else:
            raise RuntimeError("Unsupported number of quantum order event %s" % order)

    def set_name(self, name):
        if isinstance(name, str):
            self._system_name = name
            self._output_name = name
        else:
            raise RuntimeError("Invalid name. Name should be a string but it is %s " % type(name))

    def set_cross_section(self, cross_section=None):
        self._cross_section_factor = cross_section

    def case_from_scratch(self):
        """
        User performs calculation from scratch (not loaded from hdf file). All data is calculated.
        """
        Abins(
            AbInitioProgram=self._ab_initio_program,
            VibrationalOrPhononFile=self._system_name + self._extension[self._ab_initio_program],
            TemperatureInKelvin=self._temperature,
            SampleForm=self._sample_form,
            Instrument=self._instrument_name,
            BinWidthInWavenumber=self._bin_width,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            QuantumOrderEventsNumber=str(self._quantum_order_event),
            Scale=self._scale,
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=self._output_name,
        )

    def case_restart_diff_t(self):
        """
        The considered testing scenario looks as follows. First the user performs the simulation for T=10K (first run).
        Then the user changes T to 20K (second run). For T=20K  S has to be recalculated. After that the user performs
        simulation with the same parameters as for the initial simulation, e.g., T=10K (third run). In the third run all
        required data will be read from hdf file. It is checked if workspace for the initial run and the third run is
        the same (should be the same).
        """
        temperature_for_test = self._temperature + 10  # 20K
        wrk_name = self._system_name

        # T = 10 K
        Abins(
            AbInitioProgram=self._ab_initio_program,
            VibrationalOrPhononFile=self._system_name + self._extension[self._ab_initio_program],
            TemperatureInKelvin=self._temperature,
            SampleForm=self._sample_form,
            Instrument=self._instrument_name,
            BinWidthInWavenumber=self._bin_width,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            Scale=self._scale,
            QuantumOrderEventsNumber=str(self._quantum_order_event),
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=wrk_name + "init",
        )

        # T = 20 K
        Abins(
            AbInitioProgram=self._ab_initio_program,
            VibrationalOrPhononFile=self._system_name + self._extension[self._ab_initio_program],
            TemperatureInKelvin=temperature_for_test,
            SampleForm=self._sample_form,
            Instrument=self._instrument_name,
            BinWidthInWavenumber=self._bin_width,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            Scale=self._scale,
            QuantumOrderEventsNumber=str(self._quantum_order_event),
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=wrk_name + "_mod",
        )

        # T = 10 K
        Abins(
            AbInitioProgram=self._ab_initio_program,
            VibrationalOrPhononFile=self._system_name + self._extension[self._ab_initio_program],
            TemperatureInKelvin=self._temperature,
            SampleForm=self._sample_form,
            Instrument=self._instrument_name,
            BinWidthInWavenumber=self._bin_width,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            Scale=self._scale,
            QuantumOrderEventsNumber=str(self._quantum_order_event),
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=self._output_name,
        )

    def case_restart_diff_order(self, order=None):
        """
        The considered testing scenario looks as follows. First calculations are performed for
        self._quantum_order_event. Then calculations are performed for order (different quantum order event). In case
        order >  self._quantum_order_event then S should be calculated. Otherwise, it will be loaded from an hdf file.
        :param order: number of quantum order event for which restart should be done.
        """
        self.case_from_scratch()
        DeleteWorkspace(self._output_name)
        Abins(
            AbInitioProgram=self._ab_initio_program,
            VibrationalOrPhononFile=self._system_name + self._extension[self._ab_initio_program],
            TemperatureInKelvin=self._temperature,
            SampleForm=self._sample_form,
            Instrument=self._instrument_name,
            BinWidthInWavenumber=self._bin_width,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            Scale=self._scale,
            QuantumOrderEventsNumber=str(order),
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=self._output_name,
        )

    def __del__(self):
        """
        Destructor removes output files after tests and workspaces.
        :return:
        """

        try:
            abins.test_helpers.remove_output_files(list_of_names=[self._system_name])
        except TypeError:
            # nothing to remove but it is OK
            pass

        mtd.clear()


# ----------------------------------------------------------------------------------------------------------------
# Tests for 1D S
# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestScratch(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from CRYSTAL and for 1-4 quantum
    order events is correct.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "TolueneScratchAbins"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.case_from_scratch()

    def excludeInPullRequests(self):
        return True

    def validate(self):
        self.tolerance = 1e-2
        return self._output_name, self.ref_result


# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestBiggerSystem(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from CRYSTAL and for only 1 quantum
    order event is correct.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "Crystalb3lypScratchAbins"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_ONE)
        self.case_from_scratch()

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestT(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark scenario of restart is considered in which data for other temperature already exists in an hdf
    file. In this benchmark input data from CRYSTAL DFT program is used.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "TolueneTAbins"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.case_restart_diff_t()

    def excludeInPullRequests(self):
        return True

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestLargerOrder(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from restart with input data from CRYSTAL is correct. Requested order
    of quantum event is larger than the one which is saved to an hdf file so S has to be calculated.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "TolueneLargerOrderAbins"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_ONE)
        self.case_restart_diff_order(QUANTUM_ORDER_TWO)

    def excludeInPullRequests(self):
        return True

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestSmallerOrder(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from restart with input data from CRYSTAL is correct. Requested
    order of quantum event is smaller than the one which is saved to an hdf file so S is loaded from an hdf file.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "TolueneSmallerOrderAbins"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.case_restart_diff_order(QUANTUM_ORDER_ONE)

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


class AbinsCRYSTALTestScale(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if scaling is correct.
    """

    _wrk_1 = None
    _ref_result = None
    tolerance = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        scaling_factor = 2.0

        name = "TolueneScale"
        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)

        self.set_scale(scale=scaling_factor)
        self.case_from_scratch()

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


# noinspection PyAttributeOutsideInit,PyPep8Naming
class AbinsCASTEPNoH(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation for systems without H is correct.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "Na2SiF6_CASTEP"
        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CASTEP")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.set_cross_section(cross_section="Total")
        self.case_from_scratch()
        self._wrk_1 = self._output_name

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


# noinspection PyAttributeOutsideInit,PyPep8Naming
class AbinsCASTEP1DDispersion(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation of S from phonon dispersion is correct (1D case).
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "Mapi"
        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CASTEP")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_ONE)
        self.case_from_scratch()
        self._wrk_1 = self._output_name

    def validate(self):
        self.tolerance = 1e-1
        return self._output_name, self.ref_result


class AbinsDMOL3TestScratch(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from DMOL3 and for 1-4 quantum
    order events is correct.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "Na2SiF6_DMOL3"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("DMOL3")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.set_cross_section(cross_section="Total")
        self.case_from_scratch()

    def excludeInPullRequests(self):
        return True

    def validate(self):
        self.tolerance = 1e-2
        return self._output_name, self.ref_result


class AbinsGAUSSIANestScratch(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from GAUSSIAN and for 1-4 quantum
    order events is correct.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "C6H5Cl-Gaussian"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("GAUSSIAN")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.set_cross_section(cross_section="Incoherent")
        self.case_from_scratch()

    def excludeInPullRequests(self):
        return True

    def validate(self):
        self.tolerance = 1e-2
        return self._output_name, self.ref_result


class AbinsBinWidth(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation with bin width different than the default value is correct.
    Calculation performed for crystalline benzene for 1st and 2nd quantum event for output from CASTEP and bin width
    3 cm^-1. This system test should be fast so no need for excludeInPullRequests flag.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)
        name = "BenzeneBinWidthCASTEP"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CASTEP")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.set_cross_section(cross_section="Incoherent")
        self.set_bin_width(width=3.0)
        self.case_from_scratch()

    def validate(self):
        self.tolerance = 1e-2
        return self._output_name, self.ref_result


class AbinsCASTEPIsotopes(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation of the system with isotopic substitutions: H -> 2H, Li -> 7Li,
    produces correct results. Input data is generated by CASTEP. This system test should be fast so no need for
    excludeInPullRequests flag.
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)
        name = "LiOH_H2O_2D2O_CASTEP"
        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CASTEP")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_ONE)
        self.set_cross_section(cross_section="Incoherent")
        self.set_bin_width(width=2.0)
        self.case_from_scratch()

    def validate(self):
        self.tolerance = 1e-2
        return self._output_name, self.ref_result


# ------------------------------------------------------------------------------
# Tests for 2D S
# ------------------------------------------------------------------------------


class AbinsCRYSTAL2D(systemtesting.MantidSystemTest, HelperTestingClass):
    """
    Check Abins2D runs successfully for a typical use case
    """

    tolerance = None
    ref_result = None

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "AbinsCRYSTAL2D"

        self.ref_result = name + ".nxs"
        self.set_ab_initio_program("CRYSTAL")
        self.set_name(name)
        self.set_order(QUANTUM_ORDER_TWO)
        self.set_instrument_name("MARI")
        params_2d = {"Chopper": "A", "ChopperFrequency": str(300), "IncidentEnergy": str(200), "EnergyUnits": "meV"}

        Abins2D(
            AbInitioProgram="CRYSTAL",
            VibrationalOrPhononFile="TolueneScratchAbins" + self._extension[self._ab_initio_program],
            TemperatureInKelvin=self._temperature,
            Instrument=self._instrument_name,
            Atoms=self._atoms,
            SumContributions=self._sum_contributions,
            QuantumOrderEventsNumber=str(self._quantum_order_event),
            autoconvolution=True,
            ScaleByCrossSection=self._cross_section_factor,
            OutputWorkspace=self._output_name,
            **params_2d,
        )

    def validate(self):
        self.tolerance = 1e-4
        return self._output_name, self.ref_result
