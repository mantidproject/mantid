from Direct.detpackmap import sequoia
import unittest


class TestCase(unittest.TestCase):

    def test_sequoia(self):
        asserteq = self.assertEqual
        asserteq(sequoia('B1'),  38)
        asserteq(sequoia('B37'),  74)
        asserteq(sequoia('C1'),  75)
        asserteq(sequoia('C25T'),  99)
        asserteq(sequoia('C26T'),  100)
        asserteq(sequoia('C25B'),  101)
        asserteq(sequoia('C26B'),  102)
        asserteq(sequoia('C37'),  113)
        asserteq(sequoia('D1'),  114)
        asserteq(sequoia('D37'),  150)
        return


if __name__ == '__main__':
    unittest.main()
