import unittest

from mantidqt.widgets.workspacedisplay.table.error_column import ErrorColumn


class ErrorColumnTest(unittest.TestCase):

    def test_correct_init(self):
        ErrorColumn(0, 1)

    def test_raises_for_same_y_and_yerr(self):
        self.assertRaises(ValueError, lambda: ErrorColumn(2, 2))

    def test_eq_versus_ErrorColumn(self):
        ec1 = ErrorColumn(0, 1)
        ec2 = ErrorColumn(0, 1)
        self.assertEqual(ec1, ec2)

        ec1 = ErrorColumn(0, 3)
        ec2 = ErrorColumn(0, 1)
        self.assertEqual(ec1, ec2)

        ec1 = ErrorColumn(2, 3)
        ec2 = ErrorColumn(0, 3)
        self.assertEqual(ec1, ec2)

    def test_eq_versus_same_int(self):
        ec = ErrorColumn(150, 1)
        self.assertEqual(ec, 150)

    def test_eq_unsupported_type(self):
        ec = ErrorColumn(150, 1)
        self.assertRaises(RuntimeError, lambda: ec == "awd")

    def test_int(self):
        ec = ErrorColumn(1, 2)
        self.assertEqual(int(ec), 1)
