# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Sizing for the texture direction component boxes.

The Texture Planner and the Engineering Diffraction settings both lay the three sample directions
out as a grid of one-component boxes. The values are not always short: a direction carried round by
an initial shape rotation is written to full precision, so a fixed narrow box hides most of it.

These boxes are widened to fit the longest value currently in the grid, and every box in the grid is
given the same width so the columns stay aligned.
"""

from typing import Sequence

from qtpy.QtWidgets import QLineEdit

# room for a signed 3 dp component, so a grid of short values does not collapse to an unreadable box
MIN_FIT_TEXT = "-0.000"
# a full precision double is ~18 characters; beyond that the box scrolls rather than the grid growing
# without bound on a pasted or mistyped entry
MAX_FIT_TEXT = "-0.12345678901234567"
# frame, text margin and cursor space, none of which the font metrics account for
_PADDING = 12


def fit_direction_fields(fields: Sequence[QLineEdit]) -> None:
    """Give every field in the grid the width of the longest entry in it."""
    if not fields:
        return
    metrics = fields[0].fontMetrics()
    longest = max(metrics.horizontalAdvance(field.text()) for field in fields)
    width = _clamp(longest, metrics) + _PADDING
    for field in fields:
        # the .ui files cap these boxes, so the maximum has to move as well as the minimum
        field.setMinimumWidth(width)
        field.setMaximumWidth(width)


def autosize_direction_fields(fields: Sequence[QLineEdit]) -> None:
    """Size the grid now and keep it fitted as the values change.

    textChanged covers both the user typing and the values being written by the presenter, so this
    is the only hook needed."""
    for field in fields:
        field.textChanged.connect(lambda _text, f=fields: fit_direction_fields(f))
    fit_direction_fields(fields)


def _clamp(width: int, metrics) -> int:
    return max(min(width, metrics.horizontalAdvance(MAX_FIT_TEXT)), metrics.horizontalAdvance(MIN_FIT_TEXT))
