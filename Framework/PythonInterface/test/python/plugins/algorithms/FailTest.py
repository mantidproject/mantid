import unittest

class FailTest(unittest.TestCase):
    def test_fail(self):
        self.assertEqual(0, 1)

if __name__ == '__main__':
    unittest.main()