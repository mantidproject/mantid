import unittest

class FlakyTest(unittest.TestCase):
    def test_flaky(self):
        import random
        rand = random.randint(0,1)
        self.assertEqual(rand, 1)

if __name__ == '__main__':
    unittest.main()