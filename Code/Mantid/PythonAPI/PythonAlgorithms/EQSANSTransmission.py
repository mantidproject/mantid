from MantidFramework import *
from mantidsimple import *
import math

# Keep track of whether we have numpy since readX() may
# return a numpy array or a list depending on whether it
# is available.
try:
    import numpy
    HAS_NUMPY = True
except:
    HAS_NUMPY = False    

class EQSANSTransmission(PythonAlgorithm):
    """
        Calculate transmission for EQSANS
        
        Transmission counts is the signal going through a pinhole in the middle of the 
        beam stop (after hitting the sample). The number of counts is equal to the neutron
        flux times the transmission N_trans = T*Phi. The transmission T is obtained by normalizing
        the N_trans by the monitor or accelerator current. T = N_trans/current. 
        Dividing the pixel counts by T we account for transmission.         
    """
    
    # Size of the box around the beam center
    NX_TRANS_PIX = 10
    NY_TRANS_PIX = 10
    
    # To define transmission peak
    transmission_peak_to_bg_ratio = 1000
    
    # To define transmission peak mask
    min_transmission_peak_to_bg_ratio = 5
    
    def category(self):
        return "SANS"

    def name(self):
        return "EQSANSTransmission"

    def PyInit(self):
        # Input workspace
        self.declareWorkspaceProperty("InputWorkspace", "", Direction.Input)
        # Output workspace to put the transmission histo into
        self.declareProperty("OutputWorkspace", "")
        # X position of the beam center
        self.declareProperty("XCenter", 96.0)
        # Y position of the beam center
        self.declareProperty("YCenter", 128.0)
        # Transmission will be normalized to 1 if True
        self.declareProperty("NormalizeToUnity", False)


    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace")
        output_ws = self.getProperty("OutputWorkspace")
        xcenter = int(math.floor(self.getProperty("XCenter")))
        ycenter = int(math.floor(self.getProperty("YCenter")))
        normalize = self.getProperty("NormalizeToUnity")

        nx_min = xcenter - self.NX_TRANS_PIX
        nx_max = xcenter + self.NX_TRANS_PIX
        ny_min = ycenter - self.NY_TRANS_PIX
        ny_max = ycenter + self.NY_TRANS_PIX

        # Sum up all TOF bins
        Integration(input_ws, input_ws+'_int')
        
        # Find pixel with highest and lowest signal
        counts_2D = numpy.zeros([2*self.NY_TRANS_PIX+1, 2*self.NX_TRANS_PIX+1])
        
        # Smallest count in a given pixel in the region around the center
        signal_min = None
        # Highest count in a given pixel in the region around the center
        signal_max = None
        # Position of the pixel with the highest count
        xmax = xcenter
        ymax = ycenter
        
        for ix in range(nx_min, nx_max+1):
            for iy in range(ny_min, ny_max+1):
                i_pixel = 256*ix+iy
                signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
                
                if signal_min is None or signal < signal_min:
                    signal_min = signal
                if signal_max is None or signal > signal_max:
                    signal_max = signal
                    xmax = ix
                    ymax = iy
                        
        self.log().information("Transmission search [%d, %d] [%d, %d]" % (nx_min, nx_max, ny_min, ny_max))
                
        self.log().information("Max transmission counts at [%d, %d] = %g" % (xmax, ymax,signal_max))
        self.log().information("Min transmission counts = %g" % (signal_min))
        
        # Find transmission peak boundaries
        # The peak is defined by the area around the pixel with the highest count
        # that is above the background level.
        xpeakmax = nx_max
        for i in range(xmax+1, nx_max+1):
            i_pixel = 256*i+ymax
            signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
            if signal*self.transmission_peak_to_bg_ratio < signal_max:
                xpeakmax = i-1
                break
            
            i_pixel_low = 256*(i-1)+ymax
            signal_low = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_low)[0]
            if signal > signal_low:
                xpeakmax = i-1
                break
            
        xpeakmin = nx_min
        for i in range(xmax-1, nx_min+1, -1):
            i_pixel = 256*i+ymax
            signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
            if signal*self.transmission_peak_to_bg_ratio < signal_max:
                xpeakmin = i+1
                break
            
            i_pixel_high = 256*(i+1)+ymax
            signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
            if signal > signal_high:
                xpeakmin = i+1
                break
            
        ypeakmax = ny_max
        for i in range(ymax+1, ny_max+1):
            i_pixel = 256*xmax+i
            signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
            if signal*self.transmission_peak_to_bg_ratio < signal_max:
                ypeakmax = i-1
                break
            
            i_pixel_low = 256*xmax+(i-1)
            signal_low = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_low)[0]
            if signal > signal_low:
                ypeakmax = i-1
                break

        ypeakmin = ny_min
        for i in range(ymax-1, ny_min+1, -1):
            i_pixel = 256*xmax+i
            signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
            if signal*self.transmission_peak_to_bg_ratio < signal_max:
                ypeakmin = i+1
                break
            
            i_pixel_high = 256*xmax+(i-1)
            signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
            if signal > signal_high:
                ypeakmin = i+1
                break

        self.log().information("Peak range in X [%d, %d]" % (xpeakmin, xpeakmax))
        self.log().information("Peak range in Y [%d, %d]" % (ypeakmin, ypeakmax))

        # Mask monitor peak around the beam center
        # Question for JK: why do we do this?
        mask = []
        for ix in range(xpeakmin, xpeakmax+1):
            for iy in range(ypeakmin, ypeakmax+1):
                i_pixel = 256*ix+iy
                mask.append(i_pixel)
                
        for iy in range(ypeakmin, ypeakmax+1):
            for ix in range(xpeakmin, xcenter):
                i_pixel = 256*ix+iy
                signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
                i_pixel_high = 256*(ix+1)+iy
                signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
                
                if signal > signal_high and signal*self.min_transmission_peak_to_bg_ratio<signal_max: 
                    if i_pixel in mask:
                        mask.remove(i_pixel)
                else:
                    break
                
        for iy in range(ypeakmin, ypeakmax+1):
            for ix in range(xpeakmax, xcenter, -1):
                i_pixel = 256*ix+iy
                signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
                i_pixel_high = 256*(ix-1)+iy
                signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
                
                if signal > signal_high and signal*self.min_transmission_peak_to_bg_ratio<signal_max: 
                    if i_pixel in mask:
                        mask.remove(i_pixel)
                else:
                    break
                
        for iy in range(xpeakmin, xpeakmax+1):
            for ix in range(ypeakmin, ycenter):
                i_pixel = 256*ix+iy
                signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
                i_pixel_high = 256*ix+iy+1
                signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
                
                if signal > signal_high and signal*self.min_transmission_peak_to_bg_ratio<signal_max: 
                    if i_pixel in mask:
                        mask.remove(i_pixel)
                else:
                    break
                
        for iy in range(xpeakmin, xpeakmax+1):
            for ix in range(ypeakmax, ycenter, -1):
                i_pixel = 256*ix+iy
                signal = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel)[0]
                i_pixel_high = 256*ix+iy-1
                signal_high = mantid.getMatrixWorkspace(input_ws+'_int').readY(i_pixel_high)[0]
                
                if signal > signal_high and signal*self.min_transmission_peak_to_bg_ratio<signal_max: 
                    if i_pixel in mask:
                        mask.remove(i_pixel)
                else:
                    break
        
        # Sum transmission peak counts as a function of TOF
        dataX = mantid.getMatrixWorkspace(input_ws).readX(0)
        if HAS_NUMPY:
            dataX = dataX.tolist()
        nTOF = len(dataX)
        dataY = (nTOF-1)*[0]
        dataE = (nTOF-1)*[0]
        
        transmission_counts = 0
        total_pixels = 0

        # We will normalize the transmission by the accelerator current  
        proton_charge = mantid.getMatrixWorkspace(input_ws).getRun()["proton_charge"].getStatistics().mean
        duration = mantid.getMatrixWorkspace(input_ws).getRun()["proton_charge"].getStatistics().duration
        frequency = mantid.getMatrixWorkspace(input_ws).getRun()["frequency"].getStatistics().mean
        acc_current = 1.0e-12 * proton_charge * duration * frequency
        
        for itof in range(nTOF-1):
            for ix in range(xpeakmin, xpeakmax+1):
                for iy in range(ypeakmin, ypeakmax+1):
                    i_pixel = 256*ix+iy
                    signal = mantid.getMatrixWorkspace(input_ws).readY(i_pixel)[itof]
                    error = mantid.getMatrixWorkspace(input_ws).readE(i_pixel)[itof]
                    if i_pixel in mask:
                        total_pixels += 1
                        dataY[itof] += signal
                        dataE[itof] += error*error
                        transmission_counts += signal
            dataE[itof] = math.sqrt(dataE[itof])/acc_current
            dataY[itof] /= acc_current    
        
        if normalize:
            self.log().information("Normalizing the translation to average 1")
            factor = transmission_counts/acc_current/(nTOF-1)
            for itof in range(nTOF-1):
                dataY[itof] /= factor
                dataE[itof] /= factor
        
        unitX = mantid.getMatrixWorkspace(input_ws).getAxis(0).getUnit().name()
        CreateWorkspace(output_ws, dataX, dataY, dataE, NSpec=1, UnitX=unitX)
        
        total_pixels /= (nTOF-1)
        self.log().information("Total transmission counts (%d) = %g" % (total_pixels, transmission_counts))


mtd.registerPyAlgorithm(EQSANSTransmission())
