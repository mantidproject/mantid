# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from euphonic import QpointPhononModes

from .abinitioloader import AbInitioLoader
from .euphonicloader import EuphonicLoader
from abins import AbinsData


class CASTEPLoader(AbInitioLoader):
    """
    Class which handles loading files from foo.phonon output CASTEP files.
    Functions to read phonon file taken from SimulatedDensityOfStates (credits for Elliot Oram.).
    """

    @property
    def _ab_initio_program(self) -> str:
        return "CASTEP"

    def read_vibrational_or_phonon_data(self) -> AbinsData:
        """
        Reads frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic displacements
        from a <>.phonon file. Save frequencies, weights of k-point vectors, k-point vectors, amplitudes of atomic
        displacements, hash of the phonon file (hash) to <>.hdf5

        :returns:  object of type AbinsData.
        """
        modes = QpointPhononModes.from_castep(self._clerk.get_input_filename(), prefer_non_loto=True)
        data = EuphonicLoader.data_dict_from_modes(modes)
        self.save_ab_initio_data(data=data)
        return self._rearrange_data(data=data)
