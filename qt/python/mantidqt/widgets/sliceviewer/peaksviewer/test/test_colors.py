# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports
import unittest

# local imports
from mantidqt.widgets.sliceviewer.peaksviewer.colors \
    import PeakRepresentationColorSelection


class PeakRepresentationColorSelectionTest(unittest.TestCase):

    def test_default_selection_return_after_construction(self):
        default_color = 'b'
        color_selection = PeakRepresentationColorSelection(default_color)

        self.assertEqual(default_color, color_selection.marker_color)

    def test_current_attribute_is_read_only(self):
        color_selection = PeakRepresentationColorSelection('w')

        self.assertRaises(AttributeError, setattr, color_selection,
                          'marker_color', 'b')


if __name__ == '__main__':
    unittest.main()
