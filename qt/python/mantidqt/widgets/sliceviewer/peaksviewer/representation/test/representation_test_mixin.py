# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# third party imports
from mantid.py3compat.mock import MagicMock


class MissingReprClsAttribute(object):
    def __init__(self, *args, **kwargs):
        raise RuntimeError(
            "Test class should define a REPR_CLS attribute pointing to the class to test")

    @classmethod
    def create(*args, **kwargs):
        """Implemented to slice pylint warning. This should never be called."""
        pass

    def snap_to(*args, **kwargs):
        """Implemented to slice pylint warning. This should never be called."""
        pass


class PeakRepresentationMixin(object):
    """Provides a common set of tests for all PeakRepresentations.
    Add this as the last base class after unittest.TestCase in the test class
    definition.
    """
    REPR_CLS = MissingReprClsAttribute

    def test_create_stores_expected_attributes(self):
        representation = self.create_test_object()

        self.verify_expected_attributes(representation)

    def test_representation_attributes_read_only(self):
        representation = self.create_test_object()

        for name, value in (("alpha", 2), ("x", 2), ("y", 0.1), ("z", 3), ('fg_color', 'b')):
            self.assertRaises(AttributeError, setattr, representation, name, value)

    def test_snap_to_calls_painter_method_with_peak_center(self):
        representation = self.create_test_object()
        mock_painter = MagicMock()

        representation.snap_to(mock_painter)

        mock_painter.snap_to.assert_called_once_with(representation.x, representation.y,
                                                     representation.snap_width())

    def verify_expected_attributes(self, representation):
        self.assertEqual(self.x, representation.x)
        self.assertEqual(self.y, representation.y)
        self.assertEqual(self.z, representation.z)
        self.assertEqual(self.alpha, representation.alpha)
        self.assertEqual(self.fg_color, representation.fg_color)
