from __future__ import (absolute_import, division, print_function)
import numpy as np
import stresstesting
from mantid.simpleapi import Abins, mtd, DeleteWorkspace, Scale
from AbinsModules import AbinsConstants, AbinsTestHelpers


def skip_tests():
    return not hasattr(np, "einsum")


class HelperTestingClass(object):
    def __init__(self):

        self._temperature = 10  # K
        self._sample_form = "Powder"
        self._instrument_name = "TOSCA"
        self._atoms = ""
        self._sum_contributions = True
        self._cross_section_factor = "Incoherent"
        self._extension = {"CASTEP": ".phonon", "CRYSTAL": ".out"}
        self._output_name = "output_workspace"
        self._ref = "reference_workspace"
        self._scale = 1.0

        self._dft_program = None
        self._quantum_order_event = None
        self._system_name = None

    def set_instrument_name(self, instrument_name=None):

        if instrument_name in AbinsConstants.ALL_INSTRUMENTS:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Wrong instrument.")

    def set_scale(self, scale=None):

        if isinstance(scale, float) and scale > 0.0:
            self._scale = scale
        else:
            raise ValueError("Wrong scale.")

    def set_dft_program(self, dft_program=None):

        if dft_program in AbinsConstants.ALL_SUPPORTED_DFT_PROGRAMS:
            self._dft_program = dft_program
        else:
            raise RuntimeError("Unsupported DFT program: %s " % dft_program)

    def set_order(self, order=None):

        orders = [AbinsConstants.QUANTUM_ORDER_ONE, AbinsConstants.QUANTUM_ORDER_TWO,
                  AbinsConstants.QUANTUM_ORDER_THREE, AbinsConstants.QUANTUM_ORDER_FOUR]

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
        Abins(DFTprogram=self._dft_program, PhononFile=self._system_name + self._extension[self._dft_program],
              Temperature=self._temperature, SampleForm=self._sample_form, Instrument=self._instrument_name,
              Atoms=self._atoms, SumContributions=self._sum_contributions,
              QuantumOrderEventsNumber=str(self._quantum_order_event), Scale=self._scale,
              ScaleByCrossSection=self._cross_section_factor, OutputWorkspace=self._output_name)

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
        Abins(DFTprogram=self._dft_program, PhononFile=self._system_name + self._extension[self._dft_program],
              Temperature=self._temperature, SampleForm=self._sample_form, Instrument=self._instrument_name,
              Atoms=self._atoms, SumContributions=self._sum_contributions, Scale=self._scale,
              QuantumOrderEventsNumber=str(self._quantum_order_event), ScaleByCrossSection=self._cross_section_factor,
              OutputWorkspace=wrk_name + "init")

        # T = 20 K
        Abins(DFTprogram=self._dft_program, PhononFile=self._system_name + self._extension[self._dft_program],
              Temperature=temperature_for_test, SampleForm=self._sample_form, Instrument=self._instrument_name,
              Atoms=self._atoms, SumContributions=self._sum_contributions, Scale=self._scale,
              QuantumOrderEventsNumber=str(self._quantum_order_event), ScaleByCrossSection=self._cross_section_factor,
              OutputWorkspace=wrk_name + "_mod")

        # T = 10 K
        Abins(DFTprogram=self._dft_program, PhononFile=self._system_name + self._extension[self._dft_program],
              Temperature=self._temperature, SampleForm=self._sample_form, Instrument=self._instrument_name,
              Atoms=self._atoms, SumContributions=self._sum_contributions, Scale=self._scale,
              QuantumOrderEventsNumber=str(self._quantum_order_event),
              ScaleByCrossSection=self._cross_section_factor, OutputWorkspace=self._output_name)

    def case_restart_diff_order(self, order=None):
        """
        The considered testing scenario looks as follows. First calculations are performed for
        self._quantum_order_event. Then calculations are performed for order (different quantum order event). In case
        order >  self._quantum_order_event then S should be calculated. Otherwise, it will be loaded from an hdf file.
        :param order: number of quantum order event for which restart should be done.
        """
        self.case_from_scratch()
        DeleteWorkspace(self._output_name)
        Abins(DFTprogram=self._dft_program, PhononFile=self._system_name + self._extension[self._dft_program],
              Temperature=self._temperature, SampleForm=self._sample_form, Instrument=self._instrument_name,
              Atoms=self._atoms, SumContributions=self._sum_contributions, Scale=self._scale,
              QuantumOrderEventsNumber=str(order), ScaleByCrossSection=self._cross_section_factor,
              OutputWorkspace=self._output_name)

    def __del__(self):
        """
        Destructor removes output files after tests and workspaces.
        :return:
        """
        AbinsTestHelpers.remove_output_files(list_of_names=[self._system_name])
        mtd.clear()

# ----------------------------------------------------------------------------------------------------------------
# Tests for 1D S
# ----------------------------------------------------------------------------------------------------------------


class AbinsCASTEPTestScratch(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from CASTEP and for 1-4 quantum
    order events is correct.
    """
    _ref_result = None
    tolerance = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):

        HelperTestingClass.__init__(self)

        name = "BenzeneScratchAbins"

        self.ref_result = name + ".nxs"
        self.set_dft_program("CASTEP")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_FOUR)
        self.case_from_scratch()

    def validate(self):

        self.tolerance = 1e-2

        return self._output_name, self.ref_result

# ----------------------------------------------------------------------------------------------------------------


class AbinsCRYSTALTestScratch(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from scratch with input data from CRYSTAL and for only 1 quantum
    order event is correct.
    """
    _ref_result = None
    tolerance = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):

        HelperTestingClass.__init__(self)

        name = "Crystalb3lypScratchAbins"

        self.ref_result = name + ".nxs"
        self.set_dft_program("CRYSTAL")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_ONE)
        self.case_from_scratch()

    def validate(self):

        self.tolerance = 1e-1

        return self._output_name, self.ref_result

# ----------------------------------------------------------------------------------------------------------------


class AbinsCASTEPTestT(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark scenario of restart is considered in which data for other temperature already exists in an hdf
    file. In this benchmark input data from CASTEP DFT program is used.
    """
    _ref_result = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):

        HelperTestingClass.__init__(self)

        name = "BenzeneTAbins"

        self.ref_result = name + ".nxs"
        self.set_dft_program("CASTEP")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_TWO)
        self.case_restart_diff_t()

    def validate(self):
        return self._output_name, self.ref_result

# ----------------------------------------------------------------------------------------------------------------


class AbinsCASTEPTestLargerOrder(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from restart with input data from CASTEP is correct. Requested order
    of quantum event is larger than the one which is saved to an hdf file so S has to be calculated.
    """
    _ref_result = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):

        HelperTestingClass.__init__(self)

        name = "BenzeneLargerOrderAbins"

        self.ref_result = name + ".nxs"
        self.set_dft_program("CASTEP")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_TWO)
        self.case_restart_diff_order(AbinsConstants.QUANTUM_ORDER_THREE)

    def validate(self):
        return self._output_name, self.ref_result

# ----------------------------------------------------------------------------------------------------------------


class AbinsCASTEPTestSmallerOrder(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation from restart with input data from CASTEP is correct. Requested
    order of quantum event is smaller than the one which is saved to an hdf file so S is loaded from an hdf file.
    """
    _ref_result = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):

        HelperTestingClass.__init__(self)

        name = "BenzeneSmallerOrderAbins"

        self.ref_result = name + ".nxs"
        self.set_dft_program("CASTEP")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_TWO)
        self.case_restart_diff_order(AbinsConstants.QUANTUM_ORDER_ONE)

    def validate(self):
        return self._output_name, self.ref_result


class AbinsCASTEPTestScale(stresstesting.MantidStressTest, HelperTestingClass):
        """
        In this benchmark it is tested if scaling is correct.
        """
        _wrk_1 = None
        _ref_result = None

        def skipTests(self):
            return skip_tests()

        def runTest(self):
            HelperTestingClass.__init__(self)

            scaling_factor = 2.0

            name = "BenzeneScale"
            self.ref_result = name + ".nxs"
            self.set_dft_program("CASTEP")
            self.set_name(name)
            self.set_order(AbinsConstants.QUANTUM_ORDER_TWO)
            self.case_from_scratch()
            self._wrk_1 = self._output_name

            Scale(InputWorkspace=self._wrk_1,
                  OutputWorkspace=self._wrk_1,
                  Operation='Multiply',
                  Factor=scaling_factor)

        def validate(self):
            return self._output_name, self.ref_result


# noinspection PyAttributeOutsideInit,PyPep8Naming
class AbinsCASTEPNoH(stresstesting.MantidStressTest, HelperTestingClass):
    """
    In this benchmark it is tested if calculation for systems without H is correct.
    """
    _wrk_1 = None
    _ref_result = None

    def skipTests(self):
        return skip_tests()

    def runTest(self):
        HelperTestingClass.__init__(self)

        name = "Na2SiF6"
        self.ref_result = name + ".nxs"
        self.set_dft_program("CASTEP")
        self.set_name(name)
        self.set_order(AbinsConstants.QUANTUM_ORDER_FOUR)
        self.set_cross_section(cross_section="Total")
        self.case_from_scratch()
        self._wrk_1 = self._output_name

    def validate(self):
        return self._output_name, self.ref_result
