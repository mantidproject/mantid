import numpy
import mantid
from mantid.api import AnalysisDataService

# Do absorption correction

class Lattice(object):
    def __init__(self, a, b, c, alpha, beta, gamma):
        self.a = a
        self.b = b
        self.c = c
        self.alpha = alpha
        self.beta = beta
        self.gamma = gamma


def invert_matrix(np_matrix):
    """
    """
    
    # assert isinstance(np_matrix,  numpy.ndarray), 'must be a numpy array but not %s.' % type(np_matrix)
    assert np_matrix.shape == (3, 3)

    import numpy.linalg

    inv_matrix = numpy.linalg.inv(np_matrix)

    return inv_matrix


def calculate_matrix(a, b, c, alpha, beta, gamma, spice_ub):
    """
    """
    lattice = Lattice(a, b, c, alpha, beta, gamma)

    matrix_B = calculate_B_matrix(lattice)

    return matrix_B


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
    upphi[0] = numpy.sin(theta2ave/2+omg0)*numpy.cos(chiave)*numpy.cos(phiave)+numpy.cos(theta2ave/2+omg0)*numpy.sin(phiave)
    upphi[1] ==numpy.sin(theta2ave/2+omg0)*numpy.cos(chiave)*numpy.sin(phiave)-numpy.cos(theta2ave/2+omg0)*numpy.cos(phiave)
    upphi[2] ==numpy.sin(theta2ave/2+omg0)*numpy.sin(chiave)

    return upphi

def calculate_usphi(omg0, theta2ave, chiave, phiave):
    usphi=numpy.ndarray(shape=(3,), dtype='float')
    usphi[0] =numpy.sin(theta2ave/2-omg0)*numpy.cos(chiave)*numpy.cos(phiave)-numpy.cos(theta2ave/2-omg0)*numpy.sin(phiave)
    usphi[1] =numpy.sin(theta2ave/2-omg0)*numpy.cos(chiave)*numpy.sin(phiave)+numpy.cos(theta2ave/2-omg0)*numpy.cos(phiave)
    usphi[2] =numpy.sin(theta2ave/2-omg0)*numpy.sin(chiave)

    return usphi


def calculate_absorption_correction_spice(exp_number, scan_number, lattice, ub_matrix):
    """
    SPICE ub matrix
    """

    # process angles from SPICE table
    theta2ave = get_average_spice_table(exp_number, scan_number, '2theta')  # sum(theta2(:))/length(theta2);
    chiave = get_average_spice_table(exp_number, scan_number, 'chi') # sum(chi(:))/length(chi);
    phiave = get_average_spice_table(exp_number, scan_number, 'phi') # sum(phi(:))/length(phi);
    omg0 = theta2ave * 0.5

    upphi = calculate_upphi(omg0, theta2ave, chiave, phiave)
    usphi = calculate_usphi(omg0, theta2ave, chiave, phiave)
    
    matrix_B = calculate_B_matrix(lattice)

    invUB = invert_matrix(ub_matrix)
    print numpy
    print invUB
    print matrix_B
    print upphi

    upcart = numpy.dot(numpy.dot(matrix_B, invUB), upphi)
    uscart = numpy.dot(numpy.dot(matrix_B, invUB), usphi)

    print '[DB...BAT] ', upcart, uscart
    
    return upcart, uscart


def get_average_spice_table(exp_number, scan_number, col_name):
    """
    """
    spice_table_name = 'HB3A_Exp%d_%04d_SpiceTable' % (exp_number, scan_number)
    # spice_table = mtd[spice_table_name]
    spice_table = AnalysisDataService.retrieve(spice_table_name)
    
    col_index = spice_table.getColumnNames().index(col_name) 
    
    row_number = spice_table.rowCount()
    sum_item = 0.
    for i_row in range(row_number):
        sum_item += spice_table.cell(i_row, col_index)
    avg_value = sum_item / float(row_number)
    
    return avg_value
    
#
# # print get_average_spice_table(exp_number=522, scan_number=47, col_name='2theta')
#
# lattice = Lattice(4.37574, 4.37574, 10.74752,90., 90., 90.)
#
# calculate_absorption_correction_spice(522, 47, lattice)
#     # print get_average_spice_table(exp_number=522, scan_number=47, col_name='2theta')
#
#     lattice = Lattice(4.37574, 4.37574, 10.74752, 90., 90., 90.)
#
#     calculate_absorption_correction_spice(522, 47, lattice)
        