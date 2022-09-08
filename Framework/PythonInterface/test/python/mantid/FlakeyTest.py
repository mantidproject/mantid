import unittest

class FlakeyTest(unittest.TestCase):

    def test_fleaky(self):
        import random
        rand_num = random.randint(0, 2)
        print (rand_num)
        self.assertEqual(1, rand_num)

if __name__ == '__main__':
    unittest.main()