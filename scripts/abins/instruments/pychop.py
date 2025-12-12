from typing import Literal, Optional, Tuple, Union

import numpy as np
from pychop import Instruments as pychop_instruments
from pydantic import validate_call

from abins.constants import MILLI_EV_TO_WAVENUMBER
from abins.logging import get_logger, Logger
import abins.parameters
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

            logger.notice(f"Using default chopper frequency for instrument {self._name}: {chopper_frequency} Hz")

        return chopper_frequency

    def get_angle_range(self) -> Tuple[float, float]:
        return np.min(self._tthlims), np.max(self._tthlims)

    def prepare_resolution(self):
        if self._e_init not in self._polyfits:
            self._polyfit_resolution()

    def calculate_sigma(self, frequencies):
        """
        Calculates width of Gaussian resolution function.
        :return: width of Gaussian resolution function
        """
        self.prepare_resolution()
        return np.polyval(self._polyfits[self._e_init], frequencies)

    def _polyfit_resolution(self, n_values=40, order=4):
        frequencies_invcm = np.linspace(0, self._e_init, n_values, endpoint=False)
        frequencies_mev = frequencies_invcm / MILLI_EV_TO_WAVENUMBER
        ei_mev = self._e_init / MILLI_EV_TO_WAVENUMBER

        resolution_fwhm = self._pychop_instrument.getResolution(Ei_in=ei_mev, Etrans=frequencies_mev.tolist())

        resolution_sigma = resolution_fwhm / (2 * np.sqrt(2 * np.log(2)))

        fit = np.polyfit(frequencies_invcm, resolution_sigma * MILLI_EV_TO_WAVENUMBER, order)

        self._polyfits[self._e_init] = fit


@validate_call
def validate_pychop_params(
    name: str, chopper: str, chopper_frequency: str, e_i: str, energy_units: Literal["meV", "cm-1"]
) -> dict[str, str]:
    """Check chopper parameters are in valid range

    If the chopper speed is too high relative to incident this can lead to zero transmission

    Input parameters are in string form, corresponding to Abins2D algorithm inputs

    :param name: Instrument name e.g. "MARI"
    :param name: Chopper setting e.g. "A"
    :param name: Chopper frequency e.g. "350"
    :param e_i: Incident energy in energy_units
    :param energy_units: "meV" or "cm-1"

    :return: issues dict for Algorithm validation, of form {"ParamName": "Error message"}

    """

    allowed_frequencies = abins.parameters.instruments[name]["chopper_allowed_frequencies"]
    default_frequency = abins.parameters.instruments[name].get("chopper_frequency_default", None)

    if chopper_frequency == "" and default_frequency is None:
        return {"ChopperFrequency": "This instrument does not have a default chopper frequency"}

    try:
        int(chopper_frequency)
    except ValueError:
        return {"ChopperFrequency": "Invalid chopper frequency, could not cast to integer"}

    if chopper_frequency and int(chopper_frequency) not in allowed_frequencies:
        return {
            "ChopperFrequency": f"This chopper frequency is not valid for the instrument {name}. "
            "Valid frequencies: " + ", ".join([str(freq) for freq in allowed_frequencies])
        }

    EPSILON = 1e-8  # Calculation range is slightly reduced to avoid considering stationary neutrons

    energy = float(e_i)
    if energy_units == "cm-1":
        energy = energy / MILLI_EV_TO_WAVENUMBER
    pychop_instrument = pychop_instruments.Instrument(name, chopper=chopper, freq=int(chopper_frequency))

    widths = pychop_instrument.getResolution(Ei_in=energy, Etrans=[0, energy - EPSILON])

    if np.any(np.isnan(widths)):
        return {
            "ChopperFrequency": (
                "Cannot use this combination of chopper, frequency and incident energy. Use PyChop to identify a valid setting."
            )
        }

    return {}
