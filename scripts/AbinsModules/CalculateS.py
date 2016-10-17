import numpy as np
import math
from copy import copy
from IOmodule import IOmodule
from AbinsData import AbinsData
from QData import QData
from CalculateQ import CalculateQ
from InstrumentProducer import InstrumentProducer
from CalculatePowder import CalculatePowder
from CrystalData import CrystalData
from PowderData import PowderData
from SData import SData
from Instruments import Instrument
from AbinsModules import FrequencyPowderGenerator


import AbinsParameters
import AbinsConstants

class CalculateS(IOmodule, FrequencyPowderGenerator):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, sample_form=None, abins_data=None, instrument_name=None, overtones=None, combinations=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)
        @param overtones: True if overtones should be included in calculations, otherwise False
        @param combinations: True if combinations should be calculated, otherwise False
        """
        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if sample_form in AbinsConstants.all_sample_forms:
            self._sample_form = sample_form
        else:
            raise ValueError("Invalid sample form %s" % sample_form)

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        if isinstance(overtones, bool):
            self._overtones = overtones
        else:
            raise ValueError("Invalid value of overtones. Expected values are: True, False.")

        if isinstance(combinations, bool):
            self._combinations = combinations
        else:
            raise ValueError("Invalid value of combinations. Expected values are: True, False.")

        if self._overtones:
            overtones_folder = "overtones_true"
            if self._combinations:
                combinations_folder = "combinations_true"
            else:
                combinations_folder = "combinations_false"
        else:
            overtones_folder = "overtones_false"
            combinations_folder= "combinations_false"

        if instrument_name in AbinsConstants.all_instruments:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        IOmodule.__init__(self, input_filename=filename, group_name=AbinsParameters.S_data_group + "/" + self._instrument_name + "/" + self._sample_form + "/%sK" % self._temperature + "/" + overtones_folder + "/" + combinations_folder)
        FrequencyPowderGenerator.__init__(self)


    def _calculate_s(self):

        # Produce instrument object
        _instrument_producer = InstrumentProducer()
        _instrument = _instrument_producer.produceInstrument(name=self._instrument_name)

        # Calculate Q
        _q_calculator = CalculateQ(filename=self._input_filename,
                                   instrument=_instrument,
                                   sample_form=self._sample_form,
                                   k_points_data=self._abins_data.getKpointsData(),
                                   overtones=self._overtones,
                                   combinations=self._combinations)

        _q_vectors = _q_calculator.getData()

        # Powder case: calculate MSD and DW
        if self._sample_form == "Powder":

            _powder_calculator = CalculatePowder(filename=self._input_filename, abins_data=self._abins_data,
                                                 temperature=self._temperature)
            _powder_data = _powder_calculator.getData()
            _s_data = self._calculate_s_powder(q_data=_q_vectors, powder_data=_powder_data, instrument=_instrument)

        # Crystal case: calculate DW
        else:
            raise ValueError("SingleCrystal case not implemented yet.")

        return _s_data


    def _get_gamma_data(self, k_data=None):
        """
        Extracts k points data only for Gamma point.
        @param k_data: numpy array with k-points data.
        @return:
        """
        gamma_pkt_index = -1

        # look for index of Gamma point
        num_k = k_data["k_vectors"].shape[0]
        for k in range(num_k):
            if np.linalg.norm(k_data["k_vectors"][k]) < AbinsConstants.small_k:
                gamma_pkt_index = k
                break
            if gamma_pkt_index == -1:
                raise ValueError("Gamma point not found.")

        k_points= {   "weights": np.asarray([k_data["weights"][gamma_pkt_index]]),
                      "k_vectors": np.asarray([k_data["k_vectors"][gamma_pkt_index]]),
                      "frequencies": np.asarray([k_data["frequencies"][gamma_pkt_index]]),
                      "atomic_displacements": np.asarray([k_data["atomic_displacements"][gamma_pkt_index]])}
        return k_points


    # noinspection PyTypeChecker
    def _calculate_s_powder(self, q_data=None, powder_data=None, instrument=None):

        """
        Calculates S for the powder case.

        @param q_data:  data with squared Q vectors; it is an object of type QData
        @param powder_data: object of type PowderData with mean square displacements and Debye-Waller factors for the case of powder
        @param instrument: object of type Instrument; instance of the instrument for which the whole simulation is performed
        @return: object of type SData with dynamical structure factors for the powder case
        """
        if not isinstance(q_data, QData):
            raise ValueError("Input parameter 'q_data'  should be of type QData.")

        if not isinstance(powder_data, PowderData):
            raise ValueError("Input parameter 'powder_data' should be of type PowderData.")

        if not isinstance(instrument, Instrument):
            raise ValueError("Input parameter 'instrument' should be of type Instrument.")

        atom_items = {}
        powder_atom_data = powder_data.extract()
        num_atoms = powder_atom_data["msd"].shape[0]
        msd = powder_atom_data["msd"]
        dw = powder_atom_data["dw"]
        extracted_q_data = q_data.extract()
        abins_data_extracted = self._abins_data.extract()
        atom_data = abins_data_extracted["atoms_data"]
        k_points_data = self._get_gamma_data(abins_data_extracted["k_points_data"])
        fundamentals_freq = np.multiply(k_points_data["frequencies"][0], 1.0 / AbinsConstants.cm1_2_hartree)
        temperature_hartree = self._temperature * AbinsConstants.k_2_hartree

        factorials = [np.math.factorial(n) for n in range(AbinsConstants.fundamentals, AbinsConstants.fundamentals_dim + AbinsConstants.higher_order_quantum_effects_dim + AbinsConstants.s_last_index)]
        if self._overtones:
            s_total_dim = AbinsConstants.fundamentals_dim + AbinsConstants.higher_order_quantum_effects_dim
        else:
            s_total_dim = AbinsConstants.fundamentals_dim

        s = {}
        s_frequencies = {}

        # calculate frequencies
        local_frequencies = fundamentals_freq
        generated_frequencies = []

        for exponential in range(AbinsConstants.fundamentals, s_total_dim + AbinsConstants.s_last_index):
            if self._combinations:

                local_frequencies = self.construct_freq_combinations(previous_array=local_frequencies,
                                                                     fundamentals_array=fundamentals_freq,
                                                                     quantum_order=exponential)

            else:

                local_frequencies = self.construct_freq_overtones(fundamentals_array=fundamentals_freq, quantum_order=exponential)

            generated_frequencies.append(local_frequencies)

        for atom in range(num_atoms):

            for exponential in range(AbinsConstants.fundamentals, s_total_dim + AbinsConstants.s_last_index):

                q = extracted_q_data["order_%s" % exponential]
                exponential_indx = exponential - AbinsConstants.python_index_shift
                # coefficient used to evaluate S
                factor_s = 1.0 / (factorials[exponential_indx] * 4 * np.pi)
                # noinspection PyTypeChecker
                # value_dft = np.multiply(np.power(np.multiply(q, msd[atom]), exponential), np.exp(-np.multiply(q, dw[atom])))
                value_dft = np.multiply(math.e**(exponential), np.multiply(np.power(np.multiply(q, msd[atom]), exponential), np.exp(-np.multiply(q, dw[atom]))))
                # convolve value with instrumental resolution; resulting spectrum has broadened peaks with Gaussian-like shape
                freq, broad_spectrum = instrument.convolve_with_resolution_function(frequencies=generated_frequencies[exponential_indx], s_dft=value_dft)
                rebined_freq, rebined_broad_spectrum = self._rebin_data(array_x=freq, array_y=broad_spectrum)

                s["order_%s" % exponential] = np.multiply(rebined_broad_spectrum, factor_s)
                if atom == 1:

                   s_frequencies["order_%s" % exponential] = rebined_freq

            atom_items["atom_%s" % atom] = {"s": copy(s),
                                            "symbol": atom_data[atom]["symbol"],
                                            "sort":  atom_data[atom]["sort"]}

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items={"atoms_data": atom_items, "frequencies": s_frequencies})

        return s_data


    def _calculate_s_crystal(self, crystal_data=None):

        if not isinstance(crystal_data, CrystalData):
            raise ValueError("Input parameter should be of type CrystalData.")
        # TODO: implement calculation of S for the single crystal scenario


    def _rebin_data(self, array_x=None, array_y=None):
        """
        Rebins S data so that all quantum effects have the same x-axis.
        @param array_x: numpy array with frequencies
        @param array_y: numpy array with S
        @return: rebined frequencies, rebined S
        """

        bins = np.arange(AbinsParameters.min_wavenumber, AbinsParameters.max_wavenumber, AbinsParameters.bin_width)
        if bins.size > array_x.size:
            return array_x, array_y
        else:

            inds = np.digitize(array_x, bins)
            rebined_y = np.asarray([array_y[inds == i].sum() for i in range(1, bins.size)])
            return bins[1:], rebined_y


    def calculateData(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """

        data = self._calculate_s()
        self.addFileAttributes()
        self.addData("data", data.extract())
        self.save()

        return data


    def loadData(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        data = self.load(list_of_datasets=["data"], list_of_attributes=["filename"])
        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items=data["datasets"]["data"])

        return s_data

