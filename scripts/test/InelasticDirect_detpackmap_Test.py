from Direct.detpackmap import sequoia
import unittest


class TestCase(unittest.TestCase):
    def test_sequoia(self):
        assert sequoia('B1') == 38
        assert sequoia('B37') == 74
        assert sequoia('C1') == 75
        assert sequoia('C25T') == 99
        assert sequoia('C26T') == 100
        assert sequoia('C25B') == 101
        assert sequoia('C26B') == 102
        assert sequoia('C37') == 113
        assert sequoia('D1') == 114
        assert sequoia('D37') == 150
        return


if __name__ == '__main__':
    unittest.main()
