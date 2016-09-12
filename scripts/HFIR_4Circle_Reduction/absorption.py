import numpy
import numpy as np
import mantid
import math
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
        
    def __str__(self):
        return '%f, %f, %f, %f, %f, %f' % (self.a, self.b, self.c, self.alpha, self.beta, self.gamma)
        
        
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
    
def multiply_2_1(med_matrix, upphi):
    out = numpy.ndarray(shape=(3,))
    for i in range(3):
        out[i] = 0
        for j in range(3):
            out[i] += med_matrix[i,j]*upphi[j]
            
    return out


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
    """   0   0   2   12.10    3.48   1   -0.94    0.98   -0.19    0.20    0.27    0.00

    """
    return numpy.cos(degree / 180. * numpy.pi)


def calculate_B_matrix(lattice):
    """
    """
    # FIXME - why there are /(2*pi) terms in the original formula
    
    latticestar = Lattice(1,1,1,0,0,0)
    
    # calculaste lattice *
    V=2*lattice.a*lattice.b*lattice.c*numpy.sqrt(m_sin((lattice.alpha+lattice.beta+lattice.gamma)/2)*m_sin((-lattice.alpha+lattice.beta+lattice.gamma)/2)*m_sin((lattice.alpha-lattice.beta+lattice.gamma)/2)*m_sin((lattice.alpha+lattice.beta-lattice.gamma)/2))
    Vstar=(2*numpy.pi)**3./V;
    latticestar.a=2*numpy.pi*lattice.b*lattice.c*m_sin(lattice.alpha)/V
    latticestar.b=2*numpy.pi*lattice.a*lattice.c*m_sin(lattice.beta)/V
    latticestar.c=2*numpy.pi*lattice.b*lattice.a*m_sin(lattice.gamma)/V
    print 'XXX: ', (m_cos(lattice.beta)*m_cos(lattice.gamma)-m_cos(lattice.alpha))/(m_sin(lattice.beta)*m_sin(lattice.gamma)) 
    latticestar.alpha=math.acos( (m_cos(lattice.beta)*m_cos(lattice.gamma)-m_cos(lattice.alpha))/(m_sin(lattice.beta)*m_sin(lattice.gamma)) ) * 180./numpy.pi
    latticestar.beta= math.acos( (m_cos(lattice.alpha)*m_cos(lattice.gamma)-m_cos(lattice.beta))/(m_sin(lattice.alpha)*m_sin(lattice.gamma)) ) * 180./numpy.pi
    latticestar.gamma=math.acos( (m_cos(lattice.alpha)*m_cos(lattice.beta)-m_cos(lattice.gamma))/(m_sin(lattice.alpha)*m_sin(lattice.beta)) ) * 180./numpy.pi
    
    print 'Lattice * = ', latticestar
 
    B = numpy.ndarray(shape=(3,3), dtype='float')
    
    B[0, 0] = latticestar.a/(0.5*numpy.pi)
    B[0, 1] = latticestar.b*m_cos(latticestar.gamma)/(0.5*numpy.pi)
    B[0, 2] = latticestar.c*m_cos(latticestar.beta)/(0.5*numpy.pi)
    B[1, 0] = 0
    B[1, 1] = latticestar.b*m_sin(latticestar.gamma)/(0.5*numpy.pi)
    B[1, 2] = -latticestar.c*m_sin(latticestar.beta)*m_cos(latticestar.alpha)/(0.5*numpy.pi)
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

    # upcart = numpy.dot(numpy.dot(matrix_B, invUB), upphi)
    med_matrix = multiply(matrix_B, invUB)
    upcart = multiply_2_1(med_matrix, upphi)
    multiply_2_1(med_matrix, upphi)
    
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


def convert_mantid_ub_to_spice(mantid_ub):
    """
    """
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
    
    
def calculate_absorption_correction_2(exp_number, scan_number, lattice, ub_matrix):
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
    
    Q=numpy.array([[1, 0, 0], [0, 1, 0], [0, 0, 1]])
    Qc=numpy.dot(ub_matrix, Q)
    print Qc
    h1c=Qc[:,0] 
    h2c=Qc[:,1] 
    h3c=Qc[:,2]    
 
    t1c=h1c/(math.sqrt(h1c[1]**2+h1c[2]**2+h1c[0]**2))
    t2c=h2c/(math.sqrt(h2c[1]**2+h2c[2]**2+h2c[0]**2))
    t3c=h3c/(math.sqrt(h3c[1]**2+h3c[2]**2+h3c[0]**2))

    upcart = numpy.ndarray(shape=(3,))
    upcart[0]=numpy.dot(upphi, t1c)
    upcart[1]=numpy.dot(upphi, t2c) 
    upcart[2]=numpy.dot(upphi, t3c)
    
    uscart = numpy.ndarray(shape=(3,))
    uscart[0]=numpy.dot(usphi, t1c)
    uscart[1]=numpy.dot(usphi, t2c)
    uscart[2]=numpy.dot(usphi, t3c)

    # matrix_B = calculate_B_matrix(lattice)
    # invUB = invert_matrix(ub_matrix)

    # upcart = numpy.dot(numpy.dot(matrix_B, invUB), upphi)
    # med_matrix = multiply(matrix_B, invUB)
    # upcart = multiply_2_1(med_matrix, upphi)
    # multiply_2_1(med_matrix, upphi)
    
    # uscart = numpy.dot(numpy.dot(matrix_B, invUB), usphi)

    print '[DB...BAT] ', upcart, uscart
    
    return upcart, uscart
    
# Test ... ...

lattice = Lattice(4.32765, 4.32765, 11.25736, 90., 90., 90.)
ub_matrix = numpy.array([[-0.1482003, -0.0376897, 0.0665967], [-0.0494848, 0.2256107, 0.0025953],[-0.1702423, -0.0327691, -0.0587285]])
spice_ub = convert_mantid_ub_to_spice(ub_matrix)
spice_ub = numpy.array([[ -0.149514, -0.036502, 0.066258], [0.168508, 0.028803, 0.059636],  [-0.045800, 0.225134, 0.003113]])
upcart, uscart = calculate_absorption_correction_2(522, 52, lattice, spice_ub)






        