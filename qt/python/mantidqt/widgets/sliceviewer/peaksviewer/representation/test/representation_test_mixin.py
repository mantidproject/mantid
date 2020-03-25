# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

# third party imports
from mantid.kernel import V3D
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

        mock_painter.snap_to.assert_called_once_with(self.x, self.y)

    def create_test_object(self):
        self.x, self.y, self.z, self.slicepoint, \
            self.slicewidth, self.shape, self.fg_color = 0.0, 1.0, -1.0, -1.05, 10.0, None, 'b'
        self.expected_alpha = 0.5333

        return self.REPR_CLS.create(self.x, self.y, self.z, self.slicepoint, self.slicewidth,
                                    self.shape, self.fg_color)

    def verify_expected_attributes(self, representation):
        self.assertEqual(self.x, representation.x)
        self.assertEqual(self.y, representation.y)
        self.assertEqual(self.z, representation.z)
        self.assertAlmostEqual(self.expected_alpha, representation.alpha, places=4)
        self.assertEqual(self.fg_color, representation.fg_color)
