# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from pathlib import Path

import numpy as np

import abins
from abins.constants import ALL_SAMPLE_FORMS
from abins.instruments import Instrument


class SCalculatorFactory(object):
    """
    Get an instantiated S calculator. Currently available S calculators:

         * SPowderSemiEmpiricalCalculator
    """

    @staticmethod
    def init(
        *,
        filename: str,
        temperature: float,
        sample_form: str,
        abins_data: abins.AbinsData,
        instrument: Instrument,
        quantum_order_num: int,
        autoconvolution_max: int = 0,
        cache_directory: Path | None = None,
    ):
        """
        :param filename: name of input DFT file (CASTEP: foo.phonon)
        :param temperature: temperature in K for which calculation of S should be done
        :param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        :param abins_data: object of type AbinsData with data from phonon file
        :param instrument: object of type Instrument for which simulation should be performed
        :param quantum_order_num: number of quantum order events taken into account during the simulation
        :param autoconvolution_max: Convolve results with fundamentals to obtain approximate spectra up to this order
        :param cache_directory: Directory for .hdf5 cache files
        """
        if sample_form in ALL_SAMPLE_FORMS:
            if sample_form == "Powder":
                return abins.SPowderSemiEmpiricalCalculator(
                    filename=filename,
                    temperature=temperature,
                    abins_data=abins_data,
                    instrument=instrument,
                    quantum_order_num=quantum_order_num,
                    autoconvolution_max=autoconvolution_max,
                    cache_directory=cache_directory,
                )

            else:
                raise ValueError("Only implementation for sample in the form of powder is available.")
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

    @classmethod
    def write_test_data(cls, out_file, **kwargs):
        """
        Write test data e.g. for CalculateSPowder test as a simple text dump

        The format is liable to change as part of a maintenance clean-up, so is not recommended for use in user scripts.

        :param out_file: Path to output test data file
        :type out_file: str

        This method will pass all other options to :obj:`CalculateS.init()` (filename, temperature etc.)
        If you need to generate abins_data, the most convenient way of doing this is with
        AbinsData.from_calculation_data(filename, ab_initio_program)
        """

        data = cls.init(**kwargs).get_formatted_data()

        # This function returns a _copy_ of the options so we can use it as a backup
        printoptions = np.get_printoptions()

        try:
            np.set_printoptions(threshold=np.nan)
            with open(out_file, "w") as f:
                f.write(str(data.extract()))
        # We probably don't want full-sized array printing if something goes wrong,
        # so use `finally` to ensure np verbosity is reset
        finally:
            np.set_printoptions(**printoptions)
