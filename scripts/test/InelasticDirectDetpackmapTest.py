from Direct.detpackmap import sequoia
import unittest


class InelasticDirectDetpackmapTest(unittest.TestCase):

    def test_sequoia(self):
        self.assertEqual(sequoia('B1'),  38)
        self.assertEqual(sequoia('B37'),  74)
        self.assertEqual(sequoia('C1'),  75)
        self.assertEqual(sequoia('C25T'),  99)
        self.assertEqual(sequoia('C26T'),  100)
        self.assertEqual(sequoia('C25B'),  101)
        self.assertEqual(sequoia('C26B'),  102)
        self.assertEqual(sequoia('C37'),  113)
        self.assertEqual(sequoia('D1'),  114)
        self.assertEqual(sequoia('D37'),  150)
        with self.assertRaises(ValueError):
            sequoia('M38')

if __name__ == '__main__':
    unittest.main()
