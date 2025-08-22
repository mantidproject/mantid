import numpy as np

from .directinstrument import DirectInstrument


class PantherInstrument(DirectInstrument):
    """Simulated direct-geometry INS with ILL-PANTHER

    Angle range is drawn from parameters file

    Resolution fitting is in progress
    """

    def __init__(self, setting=""):
        super().__init__(setting=setting, name="PANTHER")

    def get_angle_range(self):
        return self.get_parameter("angle_range")

    def calculate_sigma(self, frequencies):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        from abins.constants import MILLI_EV_TO_WAVENUMBER

        resolution = self.get_parameter("resolution")

        ei_meV = self._e_init / MILLI_EV_TO_WAVENUMBER
        frequencies_meV = frequencies / MILLI_EV_TO_WAVENUMBER

        resolution_fwhm = (
            np.polyval(resolution["abs_meV"], frequencies_meV)
            + np.polyval(resolution["ei_dependence"], ei_meV)
            + np.polyval(resolution["ei_energy_product"], ei_meV * frequencies_meV)
        ) * MILLI_EV_TO_WAVENUMBER

        resolution_sigma = resolution_fwhm / (2 * np.sqrt(2 * np.log(2)))
        return resolution_sigma
