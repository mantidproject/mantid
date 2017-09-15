from __future__ import (absolute_import, division, print_function)
import AbinsModules


class CalculateS(object):
    """
    Class producer for generating required S calculator
    Currently available S calculators:

         * SPowderSemiEmpiricalCalculator
    """

    @staticmethod
    def init(filename=None, temperature=None, sample_form=None, abins_data=None, instrument=None,
             quantum_order_num=None):
        """
        :param filename: name of input DFT file (CASTEP: foo.phonon)
        :param temperature: temperature in K for which calculation of S should be done
        :param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        :param abins_data: object of type AbinsData with data from phonon file
        :param instrument: object of type Instrument for which simulation should be performed
        :param quantum_order_num: number of quantum order events taken into account during the simulation
        """
        if sample_form in AbinsModules.AbinsConstants.ALL_SAMPLE_FORMS:
            if sample_form == "Powder":

                return AbinsModules.SPowderSemiEmpiricalCalculator(filename=filename, temperature=temperature,
                                                                   abins_data=abins_data, instrument=instrument,
                                                                   quantum_order_num=quantum_order_num)
                # TODO: implement numerical powder averaging

            # elif sample == "SingleCrystal":  #TODO implement single crystal scenario
            else:
                raise ValueError("Only implementation for sample in the form of powder is available.")
        else:
            raise ValueError("Invalid sample form %s" % sample_form)
