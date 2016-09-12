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
        
        return
        
        
def multiply(m1, m2):
    """
    """
    m3 = numpy.ndarray(shape=(3,3), dtype='float')
    
    for i in range(3):
        for j in range(3):
            m3[i,j] = 0.
            for k in range(3):
                m3[i,j] += m1[i][k] * m2[k][j]
                
    return m3


def invert_matrix(np_matrix):
    """
    """
    import numpy
    assert isinstance(np_matrix,  numpy.ndarray), 'must be a numpy array but not %s.' % type(np_matrix)
    assert np_matrix.shape == (3, 3)

    import numpy.linalg

    inv_matrix = numpy.linalg.inv(np_matrix)
    
    print '[DB...TEST] Inverted * Original = ', numpy.linalg.det(numpy.dot(inv_matrix, np_matrix))
    
        
    # FIXME - delete this another test
    print '[DB...TEST] Local multiply = ',  numpy.linalg.det(multiply(np_matrix, inv_matrix))
 

    return inv_matrix


def calculate_matrix(a, b, c, alpha, beta, gamma, spice_ub):
    """
    """
    lattice = Lattice(a, b, c, alpha, beta, gamma)

    matrix_B = calculate_B_matrix(lattice)

    return matrix_B
    
    
def m_sin(degree):
    """
    """
    return numpy.sin(degree / 180. * numpy.pi)
    
 
def m_cos(degree):
    """
    """
    return numpy.cos(degree / 180. * numpy.pi)


def calculate_B_matrix(lattice):
    """
    """
    # FIXME - why there are /(2*pi) terms in the original formula
 
    B = numpy.ndarray(shape=(3,3), dtype='float')

    B[0, 0] = lattice.a
    B[0, 1] = lattice.b*m_cos(lattice.gamma)
    B[0, 2] = lattice.c*m_cos(lattice.beta)
    B[1, 0] = 0
    B[1, 1] = lattice.b*m_sin(lattice.gamma)
    B[1, 2] = -lattice.c*m_sin(lattice.beta)*m_cos(lattice.alpha)
    B[2, 0] = 0
    B[2, 1] = 0
    B[2, 2] = 1/lattice.c
    
    print '[DB...TEST] B matrix: determination = ',  np.linalg.det(B), '\n', B
 
    return B 

def calculate_upphi(omg0, theta2ave, chiave, phiave):
    """ Equation 58 busing paper
    """
    upphi = numpy.ndarray(shape=(3,), dtype='float')
    upphi[0] = m_sin(theta2ave*0.5+omg0)*m_cos(chiave)*m_cos(phiave)+m_cos(theta2ave*0.5+omg0)*m_sin(phiave)
    upphi[1] = m_sin(theta2ave*0.5+omg0)*m_cos(chiave)*m_sin(phiave)-m_cos(theta2ave*0.5+omg0)*m_cos(phiave)
    upphi[2] = m_sin(theta2ave*0.5+omg0)*m_sin(chiave)
    
    print '[DB...TEST] UP PHI = ', upphi, '|UP PHI| = ', numpy.dot(upphi, upphi)

    return upphi

def calculate_usphi(omg0, theta2ave, chiave, phiave):
    """ Equation 58 busing paper
    """
    usphi=numpy.ndarray(shape=(3,), dtype='float')
    usphi[0] =m_sin(theta2ave*0.5-omg0)*m_cos(chiave)*m_cos(phiave)-m_cos(theta2ave*0.5-omg0)*m_sin(phiave)
    usphi[1] =m_sin(theta2ave*0.5-omg0)*m_cos(chiave)*m_sin(phiave)+m_cos(theta2ave*0.5-omg0)*m_cos(phiave)
    usphi[2] =m_sin(theta2ave*0.5-omg0)*m_sin(chiave)
    
    print '[DB...TEST] US PHI = ', usphi, '|US PHI| = ', numpy.dot(usphi, usphi)

    return usphi


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
    
    print '[DB...TEST] Exp = %d Scan = %d:\n2theta = %f, chi = %f, phi = %f, omega = %f' % (exp_number, scan_number, theta2ave, chiave, phiave, omg0)

    upphi = calculate_upphi(omg0, theta2ave, chiave, phiave)
    usphi = calculate_usphi(omg0, theta2ave, chiave, phiave)
    
    matrix_B = calculate_B_matrix(lattice)

    invUB = invert_matrix(ub_matrix)

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
    
    
 # Test ... ...
lattice = Lattice(4.32765, 4.32765, 11.25736, 90., 90., 90.)
ub_matrix = numpy.array([[-0.1482003, -0.0376897, 0.0665967], [-0.0494848, 0.2256107, 0.0025953],[-0.1702423, -0.0327691, -0.0587285]])
upcart, uscart = calculate_absorption_correction_spice(522, 41, lattice, ub_matrix)

        