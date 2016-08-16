FLOAT_REGEX = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'


def _parse_block_header(header_match, block_count):
    """
    Parse the header of a block of frequencies and intensities

    @param header_match - the regex match to the header
    @param block_count - the count of blocks found so far
    @return weight for this block of values
    """
    # Found header block at start of frequencies
    q1, q2, q3, weight = [float(x) for x in header_match.groups()]
    q_vector = [q1, q2, q3]
    if block_count > 1 and sum(q_vector) == 0:
        weight = 0.0
    return weight, q_vector

