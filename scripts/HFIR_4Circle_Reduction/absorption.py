#pylint: disable=R0913,W0403,R0903,C0103
import numpy
import numpy.linalg
import math
from mantid.api import AnalysisDataService

import fourcircle_utility as util4

# Do absorption correction


class Lattice(object):
    """
    A simple structure-styled class to hold lattice or lattice*
    """
    def __init__(self, a, b, c, alpha, beta, gamma):
        """
        Initialization
        :param a:
        :param b:
        :param c:
        :param alpha:
        :param beta:
        :param gamma:
        """
        self._a = a
        self._b = b
        self._c = c
        self._alpha = alpha
        self._beta = beta
        self._gamma = gamma

        return

    def __str__(self):
        """
        Customized output
        :return:
        """
        return '%f, %f, %f, %f, %f, %f' % (self._a, self._b, self._c, self._alpha, self._beta, self._gamma)

    def get_a(self):
        """
        get lattice a
        :return:
        """
        return self._a

    def get_b(self):
        """
        get lattice B
        :return:
        """
        return self._b

    def get_c(self):
        """
        get lattice C
        :return:
        """
        return self._c

    def get_alpha(self):
        """
        get lattice alpha
        :return:
        """
        return self._alpha

    def get_beta(self):
        """
        get lattice beta
        :return:
        """
        return self._beta

    def get_gamma(self):
        """
        get lattice gamma
        :return:
        """
        return self._gamma

    def set_a(self, lattice_a):
        """
        set lattice parameter a
        :param lattice_a:
        :return:
        """
        self._a = lattice_a

        return

    def set_b(self, lattice_b):
        """
        set lattice parameter b
        :param lattice_b:
        :return:
        """
        self._b = lattice_b

        return

    def set_c(self, lattice_c):
        """
        set lattice parameter c
        :param lattice_c:
        :return:
        """
        self._c = lattice_c

        return

    def set_alpha(self, alpha):
        """
        set alpha
        :param alpha:
        :return:
        """
        self._alpha = alpha

        return

    def set_beta(self, beta):
        """
        set beta
        :param beta:
        :return:
        """
        self._beta = beta

        return

    def set_gamma(self, gamma):
        """
        set gamma
        :param gamma:
        :return:
        """
        self._gamma = gamma

        return


def m_sin(degree):
    """
    sin function on degree
    :param degree:
    :return:
    """
    return numpy.sin(degree / 180. * numpy.pi)


def m_cos(degree):
    """
    cosine function on degree
    :param degree:
    :return:
    """
    return numpy.cos(degree / 180. * numpy.pi)


def multiply_matrices(matrix1, matrix2):
    """
    multiply 2 matrices: same as numpy.dot()
    :param matrix1:
    :param matrix2:
    :return:
    """
    matrix3 = numpy.ndarray(shape=(3, 3), dtype='float')

    for i in range(3):
        for j in range(3):
            matrix3[i, j] = 0.
            for k in range(3):
                matrix3[i, j] += matrix1[i][k] * matrix2[k][j]

    return matrix3


def multiply_matrix_vector(matrix, vector):
    """
    multiply matrix and vector: same as numpy.dot()
    :param matrix:
    :param vector:
    :return:
    """
    out = numpy.ndarray(shape=(3,))
    for i in range(3):
        out[i] = 0
        for j in range(3):
            out[i] += matrix[i, j] * vector[j]

    return out


def invert_matrix(matrix):
    """

    :param matrix:
    :return:
    """
    # check
    assert isinstance(matrix, numpy.ndarray), 'Input must be a numpy array but not %s.' % type(matrix)
    assert matrix.shape == (3, 3)

    # invert matrix
    inv_matrix = numpy.linalg.inv(matrix)

    # test
    assert abs(numpy.linalg.det(numpy.dot(inv_matrix, matrix)) - 1.0) < 0.00001

    return inv_matrix


def calculate_reciprocal_lattice(lattice):
    """

    :param lattice:
    :return:
    """
    # check
    assert isinstance(lattice, Lattice)

    # calculate reciprocal lattice
    lattice_star = Lattice(1, 1, 1, 0, 0, 0)

    # calculate volume
    volume = 2 * lattice.get_a() * lattice.get_b() * lattice.get_c() * numpy.sqrt(
        m_sin((lattice.get_alpha() + lattice.get_beta() + lattice.get_gamma()) / 2) *
        m_sin((-lattice.get_alpha() + lattice.get_beta() + lattice.get_gamma()) / 2) *
        m_sin((lattice.get_alpha() - lattice.get_beta() + lattice.get_gamma()) / 2) *
        m_sin((lattice.get_alpha() + lattice.get_beta() - lattice.get_gamma()) / 2))
    #  v_start = (2 * numpy.pi) ** 3. / volume

    # calculate a*, b*, c*
    lattice_star.set_a(2 * numpy.pi * lattice.get_b() * lattice.get_c() * m_sin(lattice.get_alpha()) / volume)
    lattice_star.set_b(2 * numpy.pi * lattice.get_a() * lattice.get_c() * m_sin(lattice.get_beta()) / volume)
    lattice_star.set_c(2 * numpy.pi * lattice.get_b() * lattice.get_a() * m_sin(lattice.get_gamma()) / volume)

    lattice_star.set_alpha(math.acos((m_cos(lattice.get_beta()) * m_cos(lattice.get_gamma()) - m_cos(lattice.get_alpha())) /
                                     (m_sin(lattice.get_beta()) * m_sin(lattice.get_gamma()))) * 180. / numpy.pi)
    lattice_star.set_beta(math.acos((m_cos(lattice.get_alpha()) * m_cos(lattice.get_gamma()) - m_cos(lattice.get_beta())) /
                                    (m_sin(lattice.get_alpha()) * m_sin(lattice.get_gamma()))) * 180. / numpy.pi)
    lattice_star.set_gamma(math.acos((m_cos(lattice.get_alpha()) * m_cos(lattice.get_beta()) - m_cos(lattice.get_gamma())) /
                                     (m_sin(lattice.get_alpha()) * m_sin(lattice.get_beta()))) * 180. / numpy.pi)

    return lattice_star


def calculate_b_matrix(lattice):
    """
    calculate B matrix
    :param lattice:
    :return:
    """
    # check
    assert isinstance(lattice, Lattice)

    # reciprocal lattice
    lattice_star = calculate_reciprocal_lattice(lattice)
    # print 'Lattice * = ', lattice_star

    b_matrix = numpy.ndarray(shape=(3, 3), dtype='float')

    b_matrix[0, 0] = lattice_star.get_a() / (0.5 * numpy.pi)
    b_matrix[0, 1] = lattice_star.get_b() * m_cos(lattice_star.get_gamma()) / (0.5 * numpy.pi)
    b_matrix[0, 2] = lattice_star.get_c() * m_cos(lattice_star.get_beta()) / (0.5 * numpy.pi)
    b_matrix[1, 0] = 0
    b_matrix[1, 1] = lattice_star.get_b() * m_sin(lattice_star.get_gamma()) / (0.5 * numpy.pi)
    b_matrix[1, 2] = -lattice_star.get_c() * m_sin(lattice_star.get_beta()) * m_cos(lattice_star.get_alpha()) / (0.5 * numpy.pi)
    b_matrix[2, 0] = 0
    b_matrix[2, 1] = 0
    b_matrix[2, 2] = 1 / lattice.get_c()

    return b_matrix


def calculate_upphi(omg0, theta2ave, chiave, phiave):
    """ Equation 58 busing paper
    """
    up_phi = numpy.ndarray(shape=(3,), dtype='float')
    up_phi[0] = m_sin(theta2ave*0.5+omg0)*m_cos(chiave)*m_cos(phiave)+m_cos(theta2ave*0.5+omg0)*m_sin(phiave)
    up_phi[1] = m_sin(theta2ave*0.5+omg0)*m_cos(chiave)*m_sin(phiave)-m_cos(theta2ave*0.5+omg0)*m_cos(phiave)
    up_phi[2] = m_sin(theta2ave*0.5+omg0)*m_sin(chiave)

    return up_phi


def calculate_usphi(omg0, theta2ave, chiave, phiave):
    """ Equation 58 busing paper
    """
    us_phi = numpy.ndarray(shape=(3,), dtype='float')
    us_phi[0] = m_sin(theta2ave*0.5-omg0)*m_cos(chiave)*m_cos(phiave)-m_cos(theta2ave*0.5-omg0)*m_sin(phiave)
    us_phi[1] = m_sin(theta2ave*0.5-omg0)*m_cos(chiave)*m_sin(phiave)+m_cos(theta2ave*0.5-omg0)*m_cos(phiave)
    us_phi[2] = m_sin(theta2ave*0.5-omg0)*m_sin(chiave)

    return us_phi


def calculate_absorption_correction_spice(exp_number, scan_number, lattice, ub_matrix):
    """
    SPICE ub matrix -0.1482003, -0.0376897, 0.0665967,
                             -0.0494848, 0.2256107, 0.0025953,
                             -0.1702423, -0.0327691, -0.0587285,
    """
    # process angles from SPICE table
    theta2ave = get_average_spice_table(exp_number, scan_number, '2theta')  # sum(theta2(:))/length(theta2);
    chiave = get_average_spice_table(exp_number, scan_number, 'chi') # sum(chi(:))/length(chi);
    phiave = get_average_spice_table(exp_number, scan_number, 'phi') # sum(phi(:))/length(phi);
    omg0 = theta2ave * 0.5

    upphi = calculate_upphi(omg0, theta2ave, chiave, phiave)
    usphi = calculate_usphi(omg0, theta2ave, chiave, phiave)

    matrix_B = calculate_b_matrix(lattice)

    invUB = invert_matrix(ub_matrix)

    # upcart = numpy.dot(numpy.dot(matrix_B, invUB), upphi)
    med_matrix = multiply_matrices(matrix_B, invUB)
    upcart = multiply_matrix_vector(med_matrix, upphi)
    multiply_matrix_vector(med_matrix, upphi)

    uscart = numpy.dot(numpy.dot(matrix_B, invUB), usphi)

    return upcart, uscart


def get_average_spice_table(exp_number, scan_number, col_name):
    """
    """
    spice_table_name = util4.get_spice_table_name(exp_number, scan_number)
    # spice_table_name = 'HB3A_Exp%d_%04d_SpiceTable' % (exp_number, scan_number)
    # spice_table = mtd[spice_table_name]
    spice_table = AnalysisDataService.retrieve(spice_table_name)

    col_index = spice_table.getColumnNames().index(col_name)

    row_number = spice_table.rowCount()
    sum_item = 0.
    for i_row in range(row_number):
        sum_item += spice_table.cell(i_row, col_index)
    avg_value = sum_item / float(row_number)

    return avg_value


def get_average_omega(exp_number, scan_number):
    """
    Get average omega (omega-theta)
    :param exp_number:
    :param scan_number:
    :return:
    """
    # get table workspace
    spice_table_name = util4.get_spice_table_name(exp_number, scan_number)
    spice_table = AnalysisDataService.retrieve(spice_table_name)

    # column index
    col_omega_index = spice_table.getColumnNames().index('omega')
    col_2theta_index = spice_table.getColumnNames().index('2theta')

    # get the vectors
    vec_size = spice_table.rowCount()
    vec_omega = numpy.ndarray(shape=(vec_size, ), dtype='float')
    vec_2theta = numpy.ndarray(shape=(vec_size, ), dtype='float')

    for i_row in range(vec_size):
        vec_omega[i_row] = spice_table.cell(i_row, col_omega_index)
        vec_2theta[i_row] = spice_table.cell(i_row, col_2theta_index)
    # END-FOR

    vec_omega -= vec_2theta * 0.5

    return numpy.sum(vec_omega)


def convert_mantid_ub_to_spice(mantid_ub):
    """
    Convert Mantid UB matrix to SPICE ub matrix
    :param mantid_ub:
    :return:
    """
    # create SPICE UB
    spice_ub = numpy.ndarray((3, 3), 'float')

    # row 0
    for i in range(3):
        spice_ub[0, i] = mantid_ub[0, i]
    # row 1
    for i in range(3):
        spice_ub[2, i] = mantid_ub[1, i]
    # row 2
    for i in range(3):
        spice_ub[1, i] = -1.*mantid_ub[2, i]

    return spice_ub


def calculate_absorption_correction_2(exp_number, scan_number, spice_ub_matrix):
    """
    Second approach to calculate absorption correction factor without calculating B matrix
    :param exp_number:
    :param scan_number:
    :param spice_ub_matrix:
    :return:
    """
    # process angles from SPICE table
    theta2ave = get_average_spice_table(exp_number, scan_number, '2theta')  # sum(theta2(:))/length(theta2);
    chiave = get_average_spice_table(exp_number, scan_number, 'chi')  # sum(chi(:))/length(chi);
    phiave = get_average_spice_table(exp_number, scan_number, 'phi')  # sum(phi(:))/length(phi);
    avg_omega = get_average_omega(exp_number, scan_number)

    up_phi = calculate_upphi(avg_omega, theta2ave, chiave, phiave)
    us_phi = calculate_usphi(avg_omega, theta2ave, chiave, phiave)

    vec_q = numpy.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    matrix_q = numpy.dot(spice_ub_matrix, vec_q)
    h1c = matrix_q[:, 0]
    h2c = matrix_q[:, 1]
    h3c = matrix_q[:, 2]

    t1c = h1c/(math.sqrt(h1c[1]**2+h1c[2]**2+h1c[0]**2))
    t2c = h2c/(math.sqrt(h2c[1]**2+h2c[2]**2+h2c[0]**2))
    t3c = h3c/(math.sqrt(h3c[1]**2+h3c[2]**2+h3c[0]**2))

    up_cart = numpy.ndarray(shape=(3,))
    up_cart[0] = numpy.dot(up_phi, t1c)
    up_cart[1] = numpy.dot(up_phi, t2c)
    up_cart[2] = numpy.dot(up_phi, t3c)

    us_cart = numpy.ndarray(shape=(3,))
    us_cart[0] = numpy.dot(us_phi, t1c)
    us_cart[1] = numpy.dot(us_phi, t2c)
    us_cart[2] = numpy.dot(us_phi, t3c)

    return up_cart, us_cart
