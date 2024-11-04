from typing import Optional, Tuple, Union

import numpy as np
from pychop import Instruments as pychop_instruments

from abins.constants import MILLI_EV_TO_WAVENUMBER
from abins.logging import get_logger, Logger
from .directinstrument import DirectInstrument


class PyChopInstrument(DirectInstrument):
    """Simulated direct-geometry INS with PyChop

    PyChop is used to compute energy resolution as a function of energy for
    given instrument settings.

    The "tthlims" (2θ limits) data from PyChop is used to determine sampling angles.
    """

    def __init__(self, name: str = "MAPS", setting: str = "", chopper_frequency: Optional[int] = None) -> None:
        super().__init__(name=name, setting=setting)

        self._chopper = setting
        self._chopper_frequency = self._check_chopper_frequency(chopper_frequency)

        self._polyfits = {}

        self._pychop_instrument = pychop_instruments.Instrument(self._name, chopper=setting, freq=chopper_frequency)

        # Get detector 2θ limits from Pychop dataset
        self._tthlims = self._pychop_instrument.detector.tthlims

    def _check_chopper_frequency(self, chopper_frequency: Union[int, None], logger: Optional[Logger] = None) -> int:
        if chopper_frequency is None:
            chopper_frequency = self.get_parameter("chopper_frequency_default")

            logger = get_logger(logger=logger)
            assert isinstance(logger, Logger)

            logger.notice(f"Using default chopper frequency for instrument {self._name}: " f"{chopper_frequency} Hz")

        return chopper_frequency

    def get_angle_range(self) -> Tuple[float, float]:
        return np.min(self._tthlims), np.max(self._tthlims)

    def calculate_sigma(self, frequencies):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        if self._e_init not in self._polyfits:
            self._polyfit_resolution()

        return np.polyval(self._polyfits[self._e_init], frequencies)

    def _polyfit_resolution(self, n_values=40, order=4):
        frequencies_invcm = np.linspace(0, self._e_init, n_values, endpoint=False)
        frequencies_mev = frequencies_invcm / MILLI_EV_TO_WAVENUMBER
        ei_mev = self._e_init / MILLI_EV_TO_WAVENUMBER

        resolution_fwhm = self._pychop_instrument.getResolution(Ei_in=ei_mev, Etrans=frequencies_mev.tolist())

        resolution_sigma = resolution_fwhm / (2 * np.sqrt(2 * np.log(2)))

        fit = np.polyfit(frequencies_invcm, resolution_sigma * MILLI_EV_TO_WAVENUMBER, order)

        self._polyfits[self._e_init] = fit
