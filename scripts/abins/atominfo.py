"""Adaptor class for Mantid Atom, handling selection of species from mass"""

import dataclasses
from functools import cached_property
from math import isnan

from mantid.kernel import Atom, logger


@dataclasses.dataclass
class AtomInfo:
    symbol: str
    mass: float

    @cached_property
    def name(self):
        if self.nucleons_number:
            return f"{self.nucleons_number}{self.symbol}"
        return self.symbol

    @property
    def z_number(self):
        return self._mantid_atom.z_number

    @property
    def nucleons_number(self):
        return self._mantid_atom.a_number

    @cached_property
    def neutron_data(self):
        return self._mantid_atom.neutron()

    @cached_property
    def _mantid_atom(self):
        from abins.constants import MASS_EPS

        nearest_int = int(round(self.mass))
        nearest_isotope = Atom(symbol=self.symbol, a_number=nearest_int)
        standard_mix = Atom(symbol=self.symbol)

        if abs(nearest_isotope.mass - standard_mix.mass) < 1e-12:
            # items are the same: standard mix is more likely to contain data
            # (e.g. Atom('F', 19) has no neutron data but Atom('F') does)
            return standard_mix

        if abs(self.mass - standard_mix.mass) < abs(self.mass - nearest_isotope.mass):
            # Standard isotopic mixture, data should be available
            return standard_mix

        if isnan(nearest_isotope.neutron().get("coh_scatt_length_real")):
            logger.warning(
                f"Nearest isotope to atomic mass {self.mass} is "
                f"{nearest_int}{self.symbol}{nearest_isotope.z_number}, but "
                f"neutron-scattering data is not available."
            )

            if abs(self.mass - standard_mix.mass) > MASS_EPS:
                raise ValueError(f"Could not find suitable isotope data for {self.symbol} with mass {self.mass}.")

            logger.warning(f"Standard mass {standard_mix.mass} is close enough, will use data for this isotope mixture.")
            return standard_mix

        return nearest_isotope
