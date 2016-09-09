# Do absorption correction


def invert_matrix(np_matrix):
    """
    """
    assert inumpy.sinstance(np_matrix, numpy.array), 'must be a numpy array but not %s.' % type(np_matrix)
    assert np_matrix.shape == (3, 3)

    import numpy.linalg

    inv_matrix = numpy.linalg.inv(np_matrix)

    return inv_matrix


def calculate_matrix(a, b, c, alpha, beta, gamma, spice_ub):
    """
    """
    lattice = Lattice(a, b, c, alpha, beta, gamma)

    matrix_B = calculate_B_matrix(lattice)


def calculate_B_matrix(lattice):
    """
    """
    B = numpy.ndarray(shape=(3,3), dtype='float')

    B[0, 0] = lattice.a/2/numpy.pi
    B[0, 1] = lattice.b*numpy.cos(lattice.gamma)/2/numpy.pi
    B[0, 2] = lattice.c*numpy.cos(lattice.beta)/2/numpy.pi
    B[1, 0] = 0
    B[1, 1] = lattice.b*numpy.sin(lattice.gamma)/2/numpy.pi
    B[1, 2] = -lattice.c*numpy.sin(lattice.beta)*numpy.cos(lattice.alpha)/2/numpy.pi
    B[2, 0] = 0
    B[2, 1] = 0
    B[2, 2] = 1/lattice.c

    return B

def calculate_upphi(omg0, theta2ave, chiave, phiave):
    upphi = numpy.ndarray(shape=(3,), dtype='float')
    upphi[0] = numpy.sin(theta2ave/2+omg0).*numpy.cos(chiave).*cos(phiave)+numpy.cos(theta2ave/2+omg0).*numpy.sin(phiave)
    upphi[1] ==numpy.sin(theta2ave/2+omg0).*numpy.cos(chiave).*sin(phiave)-numpy.cos(theta2ave/2+omg0).*numpy.cos(phiave)
    upphi[2] ==numpy.sin(theta2ave/2+omg0).*numpy.sin(chiave)

    return upphi

def calculate_usphi(omg0, theta2ave, chiave, phiave):
    usphi=[ ]
    usphi(1)=numpy.sin(theta2ave/2-omg0).*numpy.cos(chiave).*cos(phiave)-cos(theta2ave/2-omg0).*sin(phiave)
    usphi(2)=numpy.sin(theta2ave/2-omg0).*numpy.cos(chiave).*sin(phiave)+cos(theta2ave/2-omg0).*cos(phiave)
    usphi(3)=numpy.sin(theta2ave/2-omg0).*numpy.sin(chiave)

    return usphi


def calculate_absorption_correction_spice():
    """
    SPICE ub matrix
    """

    # process angles from SPICE table
    theta2ave = get_average_spice_table('2theta')  # sum(theta2(:))/length(theta2);
    chiave = get_average_spice_table('chi') # sum(chi(:))/length(chi);
    phiave = get_average_spice_table('phi') # sum(phi(:))/length(phi);
    omg0 = theta2ave * 0.5

    upphi = calculate_upphi(omg0, theta2ave, chiave, phiave)
    usphi = calculate_usphi(omg0, theta2ave, chiave, phiave)

    upcart=B*InvUB*upphi
    uscart=B*InvUB*usphi
