# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.kernel import HTTPStatus


class HTTPStatusTest(unittest.TestCase):

    def test_types(self):
        '''Test the "common" return codes'''
        self.assertTrue(hasattr(HTTPStatus, "OK"))
        self.assertTrue(hasattr(HTTPStatus, "CREATED"))
        self.assertTrue(hasattr(HTTPStatus, "MOVED_PERMANENTLY"))
        self.assertTrue(hasattr(HTTPStatus, "FORBIDDEN"))
        self.assertTrue(hasattr(HTTPStatus, "NOT_FOUND"))
        self.assertTrue(hasattr(HTTPStatus, "INTERNAL_SERVER_ERROR"))
        self.assertTrue(hasattr(HTTPStatus, "NOT_IMPLEMENTED"))
        self.assertTrue(hasattr(HTTPStatus, "BAD_GATEWAY"))
        self.assertTrue(hasattr(HTTPStatus, "SERVICE_UNAVAILABLE"))
        # make sure invented code doesn't exist
        self.assertFalse(hasattr(HTTPStatus, "NOT_OK"))

    def test_toInt(self):
        '''Test explicit conversion to int'''
        self.assertEqual(int(HTTPStatus.OK), 200)
        self.assertNotEqual(int(HTTPStatus.OK), 201)

    def test_fromInt(self):
        self.assertEqual(HTTPStatus(200), HTTPStatus.OK)
        self.assertEqual(HTTPStatus(500), HTTPStatus.INTERNAL_SERVER_ERROR)

    def test_int_compare(self):
        '''Test comparing with int'''
        self.assertEqual(HTTPStatus.OK, 200)
        self.assertEqual(200, HTTPStatus.OK)
        self.assertNotEqual(HTTPStatus.OK, 201)

if __name__ == '__main__':
    unittest.main()
