# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from unittest import mock

from mantid.kernel import StringPropertyWithValue, BoolPropertyWithValue, FloatArrayProperty, IntArrayProperty
from mantid.api import FileProperty, MultipleFileProperty, WorkspaceGroupProperty, MatrixWorkspaceProperty

from mantidqtinterfaces.drill.model.DrillParameter import DrillParameter


class DrillParameterTest(unittest.TestCase):
    NAME = "name"

    def setUp(self):
        self.parameter = DrillParameter(self.NAME)

    def test_init(self):
        self.assertEqual(self.parameter._name, self.NAME)
        self.assertIsNone(self.parameter._value)
        self.assertIsNone(self.parameter._documentation)
        self.assertIsNone(self.parameter._type)
        self.assertIsNone(self.parameter._allowedValues)
        self.assertIsNone(self.parameter._controller)

    def test_setController(self):
        controller = mock.Mock()
        self.assertIsNone(self.parameter._controller)
        self.parameter.setController(controller)
        self.assertEqual(self.parameter._controller, controller)

    def test_initFromProperty(self):
        # FileProperty
        prop = mock.Mock(spec=FileProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.FILE_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # MultipleFileProperty
        prop = mock.Mock(spec=MultipleFileProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.FILES_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # WorkspaceGroupProperty
        prop = mock.Mock(spec=WorkspaceGroupProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.WORKSPACE_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # MatrixWorkspaceProperty
        prop = mock.Mock(spec=MatrixWorkspaceProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.WORKSPACE_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # StringPropertyWithValue
        prop = mock.Mock(spec=StringPropertyWithValue)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.COMBOBOX_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        prop = mock.Mock(spec=StringPropertyWithValue)
        prop.documentation = "doc"
        prop.allowedValues = []
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.STRING_TYPE)
        self.assertEqual(self.parameter._allowedValues, [])
        # BoolPropertyWithValue
        prop = mock.Mock(spec=BoolPropertyWithValue)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.BOOL_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # FloatArrayProperty
        prop = mock.Mock(spec=FloatArrayProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.FLOAT_ARRAY_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])
        # IntArrayProperty
        prop = mock.Mock(spec=IntArrayProperty)
        prop.documentation = "doc"
        prop.allowedValues = ["v1", "v2"]
        prop.value = "value"
        self.parameter.initFromProperty(prop)
        self.assertEqual(self.parameter._value, "value")
        self.assertEqual(self.parameter._documentation, "doc")
        self.assertEqual(self.parameter._type, self.parameter.INT_ARRAY_TYPE)
        self.assertEqual(self.parameter._allowedValues, ["v1", "v2"])

    def test_getName(self):
        name = self.parameter.getName()
        self.assertEqual(name, self.NAME)

    def test_setValue(self):
        self.assertIsNone(self.parameter._value)
        self.parameter.setValue("test")
        self.assertEqual(self.parameter._value, "test")

    def test_getValue(self):
        self.assertIsNone(self.parameter._value)
        self.parameter._value = "test"
        value = self.parameter.getValue()
        self.assertEqual(value, "test")

    def test_getType(self):
        self.assertIsNone(self.parameter._type)
        self.parameter._type = "type"
        t = self.parameter.getType()
        self.assertEqual(t, "type")

    def test_getAllowedValues(self):
        self.assertIsNone(self.parameter._allowedValues)
        self.parameter._allowedValues = ["v1", "v2"]
        values = self.parameter.getAllowedValues()
        self.assertEqual(values, ["v1", "v2"])

    def test_getDocumentation(self):
        self.assertIsNone(self.parameter._documentation)
        self.parameter._documentation = "doc"
        doc = self.parameter.getDocumentation()
        self.assertEqual(doc, "doc")


if __name__ == "__main__":
    unittest.main()
