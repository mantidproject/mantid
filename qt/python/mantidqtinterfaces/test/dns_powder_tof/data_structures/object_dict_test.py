import unittest

from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import \
    ObjectDict


class ObjectDictTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.testdic = ObjectDict()

    def test__getattr__(self):
        self.testdic['test'] = 1
        self.assertEqual(self.testdic.test, 1)

    def test__setattr__(self):
        self.testdic['test'] = 2
        self.assertEqual(self.testdic.test, 2)

    def test__init__(self):
        self.assertIsInstance(self.testdic, dict)

    def test__delattr__(self):
        self.testdic['test'] = 1
        delattr(self.testdic, 'test')
        self.assertEqual(len(self.testdic), 0)


if __name__ == '__main__':
    unittest.main()
