# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.kernel import VMD
from mantid.geometry import IMDDimension
import numpy
import unittest

from testhelpers import WorkspaceCreationHelper


class MDGeometryTest(unittest.TestCase):
    _test_ndims = 4
    _test_mdws = None

    def setUp(self):
        if self._test_mdws is None:
            signal = 3.0
            self.__class__._test_mdws = WorkspaceCreationHelper.makeFakeMDHistoWorkspace(signal, self._test_ndims)  # A type of MDGeometry

    # ====================== Success cases ==================================================
    def test_numDims_returns_expected_number(self):
        self.assertEqual(self._test_ndims, self._test_mdws.getNumDims())

    def test_numNonIntegratedDims_returns_expected_number(self):
        self.assertEqual(self._test_ndims, self._test_mdws.getNumNonIntegratedDims())

    def test_getDimension_by_index_returns_IMDDimension_object_for_valid_index(self):
        dimension = self._test_mdws.getDimension(0)  # positional
        self._check_is_dimension_with_id(dimension, "x")

    def test_getDimension_by_id_returns_IMDDimension_object(self):
        dimension = self._test_mdws.getDimensionWithId("y")
        self._check_is_dimension_with_id(dimension, "y")

    def test_getDimensionIndexByName_returns_correct_index_for_valid_name(self):
        index = self._test_mdws.getDimensionIndexByName("z")
        self.assertEqual(2, index)

    def test_getDimensionIndexById_returns_correct_index_for_valid_id(self):
        index = self._test_mdws.getDimensionIndexById("y")
        self.assertEqual(1, index)

    def test_getNonIntegratedDimensions_returns_unintegrated_dimensions_as_python_list(self):
        non_integrated = self._test_mdws.getNonIntegratedDimensions()
        self.assertTrue(isinstance(non_integrated, list))
        self.assertEqual(4, len(non_integrated))

    def test_estimateResolution_returns_1d_numpy_array_same_length_as_number_dims(self):
        resolution = self._test_mdws.estimateResolution()
        self.assertTrue(isinstance(resolution, numpy.ndarray))
        self.assertEqual(1, len(resolution.shape))
        self.assertEqual(4, resolution.shape[0])

    def test_getXDimension_returns_correct_dimension_if_workspace_has_enough_dimensions(self):
        dimension = self._test_mdws.getXDimension()
        self._check_is_dimension_with_id(dimension, "x")

    def test_getYDimension_returns_correct_dimension_if_workspace_has_enough_dimensions(self):
        dimension = self._test_mdws.getYDimension()
        self._check_is_dimension_with_id(dimension, "y")

    def test_getZDimension_returns_correct_dimension_if_workspace_has_enough_dimensions(self):
        dimension = self._test_mdws.getZDimension()
        self._check_is_dimension_with_id(dimension, "z")

    def test_getTDimension_returns_correct_dimension_if_workspace_has_enough_dimensions(self):
        dimension = self._test_mdws.getTDimension()
        self._check_is_dimension_with_id(dimension, "t")

    def test_getGeometryXML_returns_non_empty_string(self):
        xml = self._test_mdws.getGeometryXML()
        self.assertGreater(len(xml), 0)

    def test_getBasisVector_returns_VMD(self):
        basis = self._test_mdws.getBasisVector(0)
        self.assertTrue(isinstance(basis, VMD))

    def test_getOrigin_returns_VMD(self):
        origin = self._test_mdws.getOrigin()
        self.assertTrue(isinstance(origin, VMD))

    def test_original_workspace_access(self):
        self.assertFalse(self._test_mdws.hasOriginalWorkspace(0))
        self.assertEqual(0, self._test_mdws.numOriginalWorkspaces())
        self.assertRaises(RuntimeError, self._test_mdws.getOriginalWorkspace, 0)

    # ====================== Failure cases ==================================================

    def test_getDimension_by_index_raises_RuntimeError_for_invalid_index(self):
        self.assertRaises(RuntimeError, self._test_mdws.getDimension, index=self._test_ndims + 1)

    def test_getDimension_by_id_raises_ValueError_for_invalid_id(self):
        self.assertRaises(ValueError, self._test_mdws.getDimensionWithId, id="test")

    def test_getDimensionIndexByName_raises_RuntimeError_for_invalid_name(self):
        self.assertRaises(RuntimeError, self._test_mdws.getDimensionIndexByName, name="NOTANAME")

    def test_getDimensionIndexById_raises_ValueError_for_invalid_name(self):
        self.assertRaises(RuntimeError, self._test_mdws.getDimensionIndexById, id="NOTANID")

    def test_getBasisVector_raises_ValueError_for_invalid_index(self):
        self.assertRaises(ValueError, self._test_mdws.getBasisVector, index=self._test_ndims + 1)

    # ========================================================================================

    def _check_is_dimension_with_id(self, dimension, expected_id):
        self.assertTrue(isinstance(dimension, IMDDimension))
        self.assertEqual(expected_id, dimension.getDimensionId())


if __name__ == "__main__":
    unittest.main()
