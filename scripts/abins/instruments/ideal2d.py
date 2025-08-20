import numpy as np
from typing import Tuple

import abins
from .directinstrument import DirectInstrument


class Ideal2D(DirectInstrument):
    """An instrument for q-resolved 2D maps"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, name="Ideal2D", **kwargs)

    def get_q_bounds(self, pad: float = 0.0) -> Tuple[float, float]:
        params = abins.parameters.instruments[self._name]
        q_min, q_max = params.get("q_range")
        return (q_min, q_max * (1.0 + pad))

    def get_abs_q_limits(self, energy: np.ndarray) -> np.ndarray:
        return np.asarray(self.get_q_bounds())[:, np.newaxis] * np.ones_like(energy)

    def calculate_sigma(self, frequencies):
        return np.full_like(frequencies, self.get_incident_energy() * self.get_parameter("resolution"))
