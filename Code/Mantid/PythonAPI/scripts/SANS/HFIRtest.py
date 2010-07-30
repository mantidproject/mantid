"""
    Validation tests for the HFIR SANS reduction.
    
    TODO: test transmission calculation w/dark current subtraction
"""
from HFIRSANSReduction import *
# Set directory containg the test data, relative to the Mantid release directory.
TEST_DIR = "../../../Test/Data/SANS2D/"

def _read_IGOR(filepath):
    """
        Read in an HFIR IGOR output file with reduced data
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Skip first header line
        f.readline()
        for line in f:
            toks = line.split()
            try:
                q    = float(toks[0])
                iq   = float(toks[1])
                diq  = float(toks[2])
                data.append([q, iq, diq])
            except:
                print "_read_IGOR:", sys.exc_value  
    return data
    
def _read_Mantid(filepath):
    """
        Read in a CVS Mantid file
        @param filepath: path of the file to be read
    """
    data = []
    with open(filepath) as f:
        # Read Q line. Starts with 'A', so remove the first item
        qtoks = f.readline().split()
        qtoks.pop(0)
        
        # Second line is I(q), Starts with 0 to be skipped
        iqtoks = f.readline().split()
        iqtoks.pop(0)
        
        # Third and fourth lines are dummy lines
        f.readline()
        f.readline()
        
        # Fifth line is dI(q). Starts with 0 to be skipped
        diqtoks = f.readline().split()
        diqtoks.pop(0)
        
        for i in range(len(qtoks)-1):
            try:
                q   = float(qtoks[i])
                iq  = float(iqtoks[i])
                diq = float(diqtoks[i])
                data.append([q, iq, diq])
            except:
                print "_read_Mantid:", i, sys.exc_value   
    return data
    
def _verify_result(reduced_file, test_file, tolerance=1e-6):
    """
        Compare the data in two reduced data files.
        @param reduced_file: path of the Mantid-reduced file
        @param test_file: path of the IGOR-reduced file
    """
    
    # Read reduced file
    data_mantid = _read_Mantid(reduced_file)
    # Read the test data to compare with
    data_igor = _read_IGOR(test_file)
    
    # Check length
    assert(len(data_mantid)==len(data_igor))
    
    # Utility methods for manipulating the lists
    def _diff_chi2(x,y): return (x[1]-y[1])*(x[1]-y[1])/(x[2]*x[2])
    def _diff_iq(x,y): return x[1]-y[1]
    def _diff_err(x,y): return x[2]-y[2]
    def _add(x,y): return x+y
    
    # Check that I(q) is the same for both data sets
    deltas = map(_diff_iq, data_mantid, data_igor)
    delta  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta)>tolerance:
        print "Sum of I(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta), tolerance)
    
    # Then compare the errors
    deltas = map(_diff_err, data_mantid, data_igor)
    delta_err  = reduce(_add, deltas)/len(deltas)
    if math.fabs(delta_err)>tolerance:
        print "Sum of dI(q) deltas is outside tolerance: %g > %g" % (math.fabs(delta_err), tolerance)
    
    # Compute chi2 of our result relative to IGOR 
    deltas = map(_diff_chi2, data_mantid, data_igor)
    chi2  = reduce(_add, deltas)/len(data_igor)
    if chi2>10.0*tolerance:
        print "Chi2 is outside tolerance: %g > %g" % (chi2, 10.0*tolerance)
    
    print "Completed tests: delta = %g / %g  chi2 = %g" % (delta, delta_err, chi2) 
    
def test_default():   
    # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    # Reduction parameters
    method = SANSReductionMethod()
    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(method, conf, filepath=datafile)    
    reduction.reduce()
    
def test_center_by_hand():
    """
        Beam center entered by hand
        Subtract dark current
        Correct for solid angle 
        Correct for detector efficiency, excluding high/low pixels
        No transmission
        No background
    """
    # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    
    # Reduction parameters
    method = SANSReductionMethod()
    method.dark_current_filepath = TEST_DIR+"BioSANS_dark_current.xml"
    method.sensitivity_flood_filepath = TEST_DIR+"BioSANS_flood_data.xml"
    method.sensitivity_use_dark_current = True
    method.sensitivity_high = 1.5
    method.sensitivity_low = 0.5
    
    method.beam_center_method = SANSReductionMethod.BEAM_CENTER_NONE
    method.beam_center_x = 16
    method.beam_center_y = 95

    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(method, conf, filepath=datafile)    
    reduction.reduce()
    
    SaveCSV(Filename="mantid_center_by_hand.txt", InputWorkspace="Iq", Separator="\t", LineSeparator="\n")
    
    _verify_result(reduced_file="mantid_center_by_hand.txt", test_file=TEST_DIR+"reduced_center_by_hand.txt")
    
def test_center_calculated():
    """
        Beam center calculated
        Subtract dark current
        Correct for solid angle 
        Correct for detector efficiency, excluding high/low pixels
        No transmission
        No background
    """
    # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    
    # Reduction parameters
    method = SANSReductionMethod()
    method.dark_current_filepath = TEST_DIR+"BioSANS_dark_current.xml"
    method.sensitivity_flood_filepath = TEST_DIR+"BioSANS_flood_data.xml"
    method.sensitivity_use_dark_current = True
    method.sensitivity_high = 1.5
    method.sensitivity_low = 0.5
    
    method.beam_center_method = SANSReductionMethod.BEAM_CENTER_DIRECT_BEAM
    method.beam_center_filepath = TEST_DIR+"BioSANS_empty_cell.xml"

    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(method, conf, filepath=datafile)    
    reduction.reduce()
    
    SaveCSV(Filename="mantid_center_calculated.txt", InputWorkspace="Iq", Separator="\t", LineSeparator="\n")
    
    _verify_result(reduced_file="mantid_center_calculated.txt", test_file=TEST_DIR+"reduced_center_calculated.txt")
    
def test_transmission():
    """
        Beam center calculated
        No dark current subtraction
        Correct for solid angle 
        No detector efficiency correction
        No background
        Transmission calculation uses sum of counts around the beam instead
        of using an error-weighted average.
    """
    # Data file to reduce
    datafile = TEST_DIR+"BioSANS_test_data.xml"
    
    # Reduction parameters
    method = SANSReductionMethod()
    
    method.transmission_method = SANSReductionMethod.TRANSMISSION_BY_HAND
    #method.transmission_method = SANSReductionMethod.TRANSMISSION_DIRECT_BEAM
    method.transmission_sample_filepath = TEST_DIR+"BioSANS_sample_trans.xml"
    method.transmission_empty_filepath = TEST_DIR+"BioSANS_empty_trans.xml"
    method.transmission_value = 0.51944
    method.transmission_error = 0.011078
    
    method.beam_center_method = SANSReductionMethod.BEAM_CENTER_DIRECT_BEAM
    method.beam_center_filepath = TEST_DIR+"BioSANS_empty_cell.xml"

    # Instrument parameters
    conf = InstrumentConfiguration()

    reduction = SANSReduction(method, conf, filepath=datafile)    
    reduction.reduce()
    
    SaveCSV(Filename="mantid_transmission.txt", InputWorkspace="Iq", Separator="\t", LineSeparator="\n")
    
    _verify_result(reduced_file="mantid_transmission.txt", 
                   test_file=TEST_DIR+"reduced_transmission.txt",
                   tolerance = 0.1)
    
    
    
if __name__ == "__main__":
    # EXECUTE THIS FROM Mantid/release
    #test_center_calculated()
    #test_center_by_hand()
    test_transmission()
    
    
