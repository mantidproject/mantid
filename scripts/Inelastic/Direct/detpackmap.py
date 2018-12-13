sequoia_packs = ['B%s' % i for i in range(1, 37+1)] \
                + ['C%s' % i for i in range(1, 24+1)] \
                + ['C25T', 'C26T', 'C25B', 'C26B'] \
                + ['C%s' % i for i in range(27, 37+1)] \
                + ['D%s' % i for i in range(1, 37+1)]
sequoia_start_index = 38


def sequoia(name):
    return sequoia_start_index + sequoia_packs.index(name)
