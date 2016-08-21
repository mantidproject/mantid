import numpy as np

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

import AbinsParameters

class CalculateS(IOmodule):
    """
    Class for calculating S(Q, omega)
    """

    def __init__(self, filename=None, temperature=None, sample_form=None, abins_data=None, instrument_name=None):
        """
        @param filename: name of input DFT file (CASTEP: foo.phonon)
        @param temperature: temperature in K for which calculation of S should be done
        @param sample_form: form in which experimental sample is: Powder or SingleCrystal (str)
        @param abins_data: object of type AbinsData with data from phonon file
        @param instrument_name: name of instrument (str)

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

        if instrument_name in AbinsParameters.all_instruments:
            self._instrument_name = instrument_name
        else:
            raise ValueError("Unknown instrument %s" % instrument_name)

        super(CalculateS, self).__init__(input_filename=filename, group_name=AbinsParameters.S_data_group + "/" + self._instrument_name + "/" + self._sample_form +"/%sK"%self._temperature)


    def _calculate_S(self):

        # Produce instrument object
        _instrument_producer = InstrumentProducer()
        _instrument = _instrument_producer.produceInstrument(name=self._instrument_name)

        # Calculate Q
        _q_calculator = CalculateQ(filename=self._input_filename, instrument=_instrument, sample_form=self._sample_form)
        if self._instrument_name != "None":

            _q_calculator.collectFrequencies(k_points_data=self._abins_data.getKpointsData())
        _q_vectors = _q_calculator.getQvectors()

        # Powder case: calculate MSD and DW
        if self._sample_form == "Powder":

            _powder_calculator = CalculatePowder(filename=self._input_filename, abins_data=self._abins_data, temperature=self._temperature)
            _powder_data = _powder_calculator.getPowder()
            _s_data = self._calculate_s_powder(q_data=_q_vectors, powder_data=_powder_data, instrument=_instrument)

        # Crystal case: calculate DW
        else:
            raise ValueError("SingleCrystal case not implemented yet.")

        return _s_data


    def _calculate_s_powder(self, q_data=None, powder_data=None, instrument=None):

        """
        Calculates S for the powder case.

        @param q_data:  data with Q vectors (or squared q vectors); it is an object of type QData
        @param powder_data: object of type PowderData with mean square displacements and Debye-Waller factors for the case of powder
        @param instrument: object of type instrument; instance of the instrument for which the whole simulation is performed
        @return: object of type SData with dynamical structure factor for the powder case
        """
        if not isinstance(q_data, QData):
            raise ValueError("Input parameter 'q_data'  should be of type QData." )

        if not isinstance(powder_data, PowderData):
            raise ValueError("Input parameter 'powder_data' should be of type PowderData.")

        if not isinstance(instrument, Instrument):
            raise ValueError("Input parameter 'instrument' should be of type Instrument.")

        _items = []

        _powder_atom_data = powder_data.extract()
        _num_atoms = _powder_atom_data["msd"].shape[0]
        _msd = _powder_atom_data["msd"]
        _dw = _powder_atom_data["dw"]

        _extracted_q_data = q_data.extract()

        _abins_data_extracted = self._abins_data.extract()
        _atom_data = _abins_data_extracted["atoms_data"]
        _k_points_data = _abins_data_extracted["k_points_data"]
        _num_k = _k_points_data["k_vectors"].shape[0]
        _num_freq = _k_points_data["frequencies"].shape[1]

        _factorials = [np.math.factorial(n) for n in range(AbinsParameters.overtones_num)]

        _value = np.zeros((AbinsParameters._pkt_per_peak * _num_freq, AbinsParameters.overtones_num + 1), dtype=AbinsParameters.float_type)
        _frequencies = np.zeros(AbinsParameters._pkt_per_peak * _num_freq, dtype=AbinsParameters.float_type)

        _value_dft = np.zeros(_num_freq, dtype=AbinsParameters.float_type) # DFT discrete peaks
        _s_sum = AbinsParameters.overtones_num

        for atom in range(_num_atoms):

            _value.fill(0.0)

            for overtone in range(AbinsParameters.overtones_num):
                # COMMENT: at the moment only valid for Gamma point calculations

                # TODO: calculation for the whole FBZ
                for k in range(_num_k):
                    _value_dft.fill(0.0)
                    # correction for acoustic modes at Gamma point
                    if np.linalg.norm(_abins_data_extracted["k_points_data"]["k_vectors"][k]) < AbinsParameters.small_k: start = 3
                    else: start = 0

                    # noinspection PyTypeChecker
                    # only discrete values
                    # comment:  (_extracted_q_data[k][start:] * _dw[atom]) are equivalent to DW from  "Vibrational spectroscopy with neutrons...."

                    _value_dft[start:] = np.power(np.multiply(_extracted_q_data[k][start:], _msd[atom]),  overtone) * np.exp( -np.multiply(_extracted_q_data[k][start:], _dw[atom]))

                    # convolve value with instrumental resolution; resulting spectrum has broadened peaks with Gaussian-like shape
                    np.add(instrument.convolve_with_resolution_function(frequencies= np.multiply(_k_points_data["frequencies"][k], 1.0 / AbinsParameters.cm1_2_hartree),
                                                                        s_dft=_value_dft,
                                                                        points_per_peak=AbinsParameters._pkt_per_peak,
                                                                        start=start),
                           _value[:, overtone],
                           _value[:, overtone])


                    np.divide(_value[:, overtone],  _factorials[overtone] * 4 * np.pi, _value[:, overtone]) # overtone contribution
                np.add(_value[:, _s_sum], _value[:, overtone], _value[:, _s_sum])  # total contribution

            _items.append({"sort":  _atom_data[atom]["sort"],
                           "symbol": _atom_data[atom]["symbol"],
                           "value":  np.copy(_value)})

        # TODO: To be changed in the future when calculation of symmetry operations is ready
        for k in range(_num_k):

            # correction for acoustic modes at Gamma point
            if np.linalg.norm(_abins_data_extracted["k_points_data"]["k_vectors"][k]) < AbinsParameters.small_k: start = 3
            else: start = 0

            np.add(_frequencies,
                   np.multiply(instrument.produce_abscissa(frequencies=np.multiply(_k_points_data["frequencies"][k], 1.0 / AbinsParameters.cm1_2_hartree),
                                                           points_per_peak=AbinsParameters._pkt_per_peak,
                                                           start=start),
                               _k_points_data["weights"][k]),
                   _frequencies)


        # Variable items is a list. Each entry in the list corresponds to one atom. Each  entry has a form of dictionary
        # with the following entries:
        #
        #     sort:    defines symmetry equivalent atoms (not functional at the moment)
        #     symbol:  symbol of atom (hydrogen -> H)
        #     value:   value  of dynamical structure factor for the given atom in powder scenario; first
        #              Constants.overtones_num dimensions stores dynamical structure factor S for overtones for the
        #              given atom. The last index (value[:, last_index]) stores sum over all overtones for the given
        #              atom.
        #

        _s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        _s_data.set(items=dict(atoms_data=_items, convoluted_frequencies=_frequencies))

        return _s_data


    def _calculate_s_crystal(self, crystal_data=None):

        if not isinstance(crystal_data, CrystalData):
            raise ValueError("Input parameter should be of type CrystalData.")
        # TODO: implement calculation of S for the single crystal scenario






    def getS(self):
        """
        Calculates dynamical structure factor S.
        @return: object of type SData and dictionary with total S.
        """

        data = self._calculate_S()

        self.addAttribute("filename", self._input_filename)
        extracted_data = data.extract()
        self.addStructuredDataset("atoms_data", extracted_data["atoms_data"])
        self.addNumpyDataset("convoluted_frequencies", extracted_data["convoluted_frequencies"])

        self.save()

        return data





    def loadData(self):
        """
        Loads S from an hdf file.
        @return: object of type SData.
        """
        _data = self.load(list_of_structured_datasets=["atoms_data"],
                          list_of_numpy_datasets=["convoluted_frequencies"],
                          list_of_attributes=["filename"])
        _s_data = SData(temperature=self._temperature, sample_form=self._sample_form)
        _s_data.set(items=dict(atoms_data=_data["structured_datasets"]["atoms_data"],
                               convoluted_frequencies=_data["datasets"]["convoluted_frequencies"]))

        return _s_data

