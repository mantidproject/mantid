def sequoia(name):
    rows = 'BCD'
    startindexes = dict(B=37, C=74, D=113)
    name = name.strip().upper()
    row = name[0]
    assert row in rows, "Invalid pack name %r" % name
    start = startindexes[row]
    col = int(name[1:3])
    # C25T, ...
    extra = 0
    if row == 'C':
        if len(name) == 4:
            assert name[-1] in 'TB', "Invalid pack name %r" % name
            if name[3] == 'B': extra = 2
        if col>26: extra = 2
    return start + col + extra


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
        assert sequoia('D1') == 114
        assert sequoia('D37') == 150
        return


if __name__ == '__main__': unittest.main()

    
