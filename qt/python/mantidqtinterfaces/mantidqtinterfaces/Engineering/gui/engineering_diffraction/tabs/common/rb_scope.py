# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations


class RbScope:
    """The active RB number, held in one place and shared by every presenter that needs it."""

    def __init__(self, rb_num: str | None = None):
        self._rb_num: str | None = None
        self.set(rb_num)

    @property
    def rb_num(self) -> str | None:
        return self._rb_num

    def set(self, rb_num: str | None) -> bool:
        """Update the active RB. Returns True if the normalised value actually changed.

        The return value matters because this is trigger onTextChanged (every keystroke),
        so callers need to know if there has been a change
        """
        normalised = (rb_num or "").strip() or None
        if normalised == self._rb_num:
            return False
        self._rb_num = normalised
        return True


class RbScopeConsumer:
    """Mixin for presenters with access methods to the RB scope"""

    _rb_scope: RbScope

    @property
    def rb_num(self) -> str | None:
        return self._rb_scope.rb_num

    @rb_num.setter
    def rb_num(self, rb_num: str | None) -> None:
        self._rb_scope.set(rb_num)

    def set_rb_scope(self, rb_scope: RbScope) -> None:
        self._rb_scope = rb_scope

    def set_rb_num(self, rb_num: str | None) -> None:
        self._rb_scope.set(rb_num)
