# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common import INSTRUMENT_DICT

DEFAULT_INSTRUMENT = INSTRUMENT_DICT[0]


class InstrumentScope:
    """The active instrument, held in one place and shared by every presenter that needs it."""

    def __init__(self, instrument: str = DEFAULT_INSTRUMENT):
        self._instrument = instrument

    @property
    def instrument(self) -> str:
        return self._instrument

    def set(self, instrument: str) -> bool:
        """Update the active instrument. Returns True if the value actually changed."""
        if instrument == self._instrument:
            return False
        self._instrument = instrument
        return True

    def set_from_index(self, instrument_index: int) -> bool:
        """Update from the position of the main window's instrument combo box."""
        return self.set(INSTRUMENT_DICT[instrument_index])


class InstrumentScopeConsumer:
    """Mixin for presenters with access methods to the instrument scope"""

    _instrument_scope: InstrumentScope

    @property
    def instrument(self) -> str:
        return self._instrument_scope.instrument

    @instrument.setter
    def instrument(self, instrument: str) -> None:
        self._instrument_scope.set(instrument)

    def set_instrument_scope(self, instrument_scope: InstrumentScope) -> None:
        self._instrument_scope = instrument_scope

    def set_instrument_override(self, instrument_index: int) -> None:
        """Called by the main window when the instrument combo box changes.

        Unlike the RB number, every consumer has something to do when the instrument changes, so
        the shared value is updated here and the reaction is left to _on_instrument_changed.
        """
        self._instrument_scope.set_from_index(instrument_index)
        self._on_instrument_changed()

    def _on_instrument_changed(self) -> None:
        """Hook for presenters that have to react to a new instrument. Does nothing by default."""
