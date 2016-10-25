###=============General Regex strings===============###

FLOAT_REGEX = r'\-?(?:\d+\.?\d*|\d*\.?\d+)'

###=============Phonon Regex strings===============###

# Header regex. Looks for lines in the following format:
#     q-pt=    1    0.000000  0.000000  0.000000      1.0000000000    0.000000  0.000000  1.000000
PHONON_HEADER_REGEX = r"^ +q-pt=\s+\d+ +(%(s)s) +(%(s)s) +(%(s)s) (?: *(%(s)s)){0,4}" % {'s': FLOAT_REGEX}


PHONON_EIGENVEC_REGEX = r"\s*Mode\s+Ion\s+X\s+Y\s+Z\s*"

###=============Castep Regex strings===============###

# Header regex. Looks for lines in the following format:
# +  q-pt=    1 (  0.000000  0.000000  0.000000)     1.0000000000              +
CASTEP_HEADER_REGEX = r" +\+ +q-pt= +\d+ \( *(?: *(%(s)s)) *(%(s)s) *(%(s)s)\) +(%(s)s) +\+" % {'s' : FLOAT_REGEX}

# Data regex. Looks for lines in the following format:
#     +     1      -0.051481   a          0.0000000  N            0.0000000  N     +
CASTEP_DATA_REGEX = r" +\+ +\d+ +(%(s)s)(?: +\w)? *(%(s)s)? *([YN])? *(%(s)s)? *([YN])? *\+" % {'s': FLOAT_REGEX}

# Atom bond regex. Looks for lines in the following format:
#   H 006 --    O 012               0.46        1.04206
CASTEP_BOND_REGEX = r" +([A-z])+ +([0-9]+) +-- +([A-z]+) +([0-9]+) +(%(s)s) +(%(s)s)" % {'s': FLOAT_REGEX}

###===============================================###


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
