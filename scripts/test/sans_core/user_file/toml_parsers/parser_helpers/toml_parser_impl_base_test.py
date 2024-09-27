# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans_core.user_file.parser_helpers.toml_parser_impl_base import TomlParserImplBase, MissingMandatoryParam


class TomlParserImplBaseTest(unittest.TestCase):
    def test_get_mandatory_key_returns_key(self):
        # Should be able to handle an explicit None value
        expected_mock = mock.NonCallableMock()
        in_dict = {"is": {"there": None}, "mock": expected_mock}
        instance = TomlParserImplBase(in_dict)
        self.assertIsNone(instance.get_mandatory_val(["is", "there"]))
        self.assertEqual(expected_mock, instance.get_mandatory_val(["mock"]))

    def test_get_mandatory_key_with_mixed_types(self):
        # Should be able to handle an explicit None value
        expected_mock = mock.NonCallableMock()
        in_dict = {"1": {"2": expected_mock}}
        instance = TomlParserImplBase(in_dict)
        self.assertEqual(expected_mock, instance.get_mandatory_val(["1", 2]))

    def test_get_mandatory_key_throws(self):
        in_dict = {"here": None}
        with self.assertRaisesRegex(MissingMandatoryParam, "this.not_there.foo"):
            TomlParserImplBase(in_dict).get_mandatory_val(["this", "not_there", "foo"])

        try:
            TomlParserImplBase(in_dict).get_mandatory_val(["not_there"])
        except KeyError:
            # Should be catchable with KeyError too, though assert raises
            # does not allow us to do isinstance so rely on thrown exception causing failure
            pass

    def test_get_val_with_existing_dict(self):
        expected = mock.NonCallableMock()
        input_dict = {"there": None}
        injected_dict = {"there": expected}

        ret = TomlParserImplBase(input_dict).get_val("there", injected_dict)
        self.assertEqual(expected, ret, "Using internal dict instead of injected")

    def test_get_val_with_list(self):
        expected = mock.NonCallableMock()
        input_dict = {"is": {"there": expected}}
        self.assertEqual(expected, TomlParserImplBase(input_dict).get_val(["is", "there"]))

    def test_get_val_returns_none_for_missing(self):
        input_dict = {"foo": 1}
        self.assertIsNone(TomlParserImplBase(input_dict).get_val("bar"))

    def test_returns_default_none(self):
        input_dict = {"foo": 1}
        expected = mock.NonCallableMock()

        self.assertEqual(expected, TomlParserImplBase(input_dict).get_val("bar", default=expected))


if __name__ == "__main__":
    unittest.main()
