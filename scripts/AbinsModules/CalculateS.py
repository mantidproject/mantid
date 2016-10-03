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
from PowderData import  PowderData
from SData import SData
from Instruments import Instrument
import AbinsModules.FrequencyGenerator as FrequencyGenerator

import AbinsParameters

class CalculateS(IOmodule):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, sample_form=None, abins_data=None, instrument_name=None, overtones=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)
        @param overtones: True if overtones should be included in calculations, otherwise False
        """
        if not (isinstance(temperature, float) or isinstance(temperature, int)):
            raise ValueError("Invalid value of the temperature. Number was expected.")
        if temperature < 0:
            raise ValueError("Temperature cannot be negative.")
        self._temperature = float(temperature)

        if  sample_form in AbinsParameters.all_sample_forms:
            self._sample_form = sample_form
        else:
            raise  ValueError("Invalid sample form %s"%sample_form)

        if isinstance(abins_data, AbinsData):
            self._abins_data = abins_data
        else:
            raise ValueError("Object of type AbinsData was expected.")

        if isinstance(overtones, bool):
            self._overtones = overtones
        else:
            raise ValueError("Invalid value of overtones. Expected values are: True, False ")

        if self._overtones:
            overtones_folder = "overtones_true"
        else:
            overtones_folder = "overtones_false"

        if instrument_name in AbinsParameters.all_instruments:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        super(CalculateS, self).__init__(input_filename=filename, group_name=AbinsParameters.S_data_group + "/" + self._instrument_name + "/" + self._sample_form + "/%sK"%self._temperature + "/" + overtones_folder)


    def _calculate_s(self):

        # Produce instrument object
        _instrument_producer = InstrumentProducer()
        _instrument = _instrument_producer.produceInstrument(name=self._instrument_name)

        # Calculate Q
        _q_calculator = CalculateQ(filename=self._input_filename,
                                   instrument=_instrument,
                                   sample_form=self._sample_form,
                                   k_points_data=self._abins_data.getKpointsData(),
                                   overtones=self._overtones)

        _q_vectors = _q_calculator.getData()

        # Powder case: calculate MSD and DW
        if self._sample_form == "Powder":

            _powder_calculator = CalculatePowder(filename=self._input_filename, abins_data=self._abins_data, temperature=self._temperature)
            _powder_data = _powder_calculator.getData()

            _s_data = self._calculate_s_powder(q_data=_q_vectors, powder_data=_powder_data, instrument=_instrument)

        # Crystal case: calculate DW
        else:
            raise ValueError("SingleCrystal case not implemented yet.")

        return _s_data

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
            raise ValueError("Input parameter 'q_data'  should be of type QData." )

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
        k_points_data = abins_data_extracted["k_points_data"]
        num_phonons = k_points_data["frequencies"][0].shape[0] # number of phonons
        fundamentals_freq = np.multiply(k_points_data["frequencies"][0], 1.0 / AbinsParameters.cm1_2_hartree)
        last_overtone = 1 # range function is inclusive with respect to the last element so need to add this
        factorials = [np.math.factorial(n) for n in range(AbinsParameters.fundamentals, AbinsParameters.fundamentals + AbinsParameters.higher_order_quantum_effects + last_overtone)]

        if self._overtones:
            s_total_dim = AbinsParameters.fundamentals + AbinsParameters.higher_order_quantum_effects
        else:
            s_total_dim = AbinsParameters.fundamentals

        #value_dft = np.zeros(shape=num_phonons, dtype=AbinsParameters.float_type) # DFT discrete peaks
        # value_dft = np.zeros(shape=(s_total_dim, FrequencyGenerator.expand_freq_dim(quantum_order=s_total_dim, num_phonons=num_phonons)), dtype=AbinsParameters.float_type)
        # num_broaden_freq = FrequencyGenerator.expand_freq_dim(quantum_order=s_total_dim, num_phonons=num_phonons) * AbinsParameters.pkt_per_peak
        # s =  np.zeros(shape=(s_total_dim, num_broaden_freq), dtype=AbinsParameters.float_type) # stores S  for one atom (fundamentals and overtones)
        # s_frequencies = np.zeros(shape=(s_total_dim, num_broaden_freq), dtype=AbinsParameters.float_type) #  x-coordinates  for the given atom  for  fundamentals and overtones

        s = {"order_1":None}
        s_frequencies = {"order_1":None}

        start = 3 # correction for acoustic modes at Gamma point

        for atom in range(num_atoms):

            local_frequencies = None
            prev_array = None
            for exponential in range(AbinsParameters.fundamentals, s_total_dim + last_overtone):
                exponential_indx = exponential - AbinsParameters.python_index_shift
                q =  extracted_q_data["order_%s"%exponential][start:]

                # coefficient used to evaluate S
                factor_s = 1.0 / (factorials[exponential_indx] * 4 * np.pi)

                # noinspection PyTypeChecker
                value_dft = np.multiply(np.power(np.multiply(q, msd[atom]), exponential), np.exp(-np.multiply(q, dw[atom])))

                local_frequencies =  FrequencyGenerator.expand_freq(previous_array=prev_array, reference_array=fundamentals_freq, quantum_order=exponential, start=start)
                prev_array = local_frequencies

                # convolve value with instrumental resolution; resulting spectrum has broadened peaks with Gaussian-like shape
                freq, broad_spectrum = instrument.convolve_with_resolution_function(frequencies=local_frequencies, s_dft=np.copy(value_dft), start=start)
                if atom == 0:

                    s_frequencies["order_%s"%exponential] = np.copy(freq)

                s["order_%s"%exponential] = np.multiply(broad_spectrum, factor_s)

            # correction to low frequencies (quasi inelastic region)
            self._low_freq_correction(extracted_q_data=extracted_q_data,
                                      extracted_frequencies=fundamentals_freq,
                                      msd=msd, dw=dw, instrument=instrument, atom=atom, s=s)

            atom_items["atom_%s"%atom] = {"s": copy(s),
                                          "symbol": atom_data[atom]["symbol"],
                                          "sort":  atom_data[atom]["sort"]}

        s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        s_data.set(items={"atoms_data": atom_items, "frequencies": s_frequencies})

        return s_data

    def _low_freq_correction(self, extracted_q_data=None, extracted_frequencies=None, dw=None, msd=None, instrument=None, atom=None, s=None):
        """
        Adds correction for low frequencies for the first order quantum effects.

        @param extracted_q_data: numpy array with Q data
        @param value_dft:  numpy array in which discrete S from DFT should be stored
        @param extracted_frequencies: numpy array with extracted frequencies from DFT
        @param dw:  numpy array with Debye-Waller factors
        @param msd: numpy array with mean square displacements
        @param instrument:  object of type Instrument
        @param atom: number of atom
        @param s: numpy array in which S data should be stored
        """

        index = 0
        end = 0
        start = 3 # correction for acoustic modes at Gamma point
        # look only at Gamma  point (first k-point is assumed to be Gamma point)
        while extracted_frequencies[end] < AbinsParameters.low_freq:
            end += 1
        if end != 0:
            q = extracted_q_data["order_1"][start:]

            # coefficient used to evaluate S
            factor_s = 1.0 /  (16.0 * np.pi)

            # noinspection PyTypeChecker
            value_dft =  np.exp(-np.multiply(q, dw[atom]))

            # convolve value with instrumental resolution; resulting spectrum has broadened peaks with Gaussian-like shape
            freq, broad_spectrum = instrument.convolve_with_resolution_function(frequencies=extracted_frequencies, s_dft=value_dft, start=start)

            broad_spectrum[end * AbinsParameters.pkt_per_peak:].fill(0)
            # store frequencies only for one atom to save memory and speed up (for other atoms is the same)
            s["order_1"][:broad_spectrum.size] += np.multiply(broad_spectrum, factor_s)


    def _calculate_s_crystal(self, crystal_data=None):

        if not isinstance(crystal_data, CrystalData):
            raise ValueError("Input parameter should be of type CrystalData.")
        # TODO: implement calculation of S for the single crystal scenario


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

