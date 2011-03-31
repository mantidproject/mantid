from reduction import Instrument
import MantidFramework

class HFIRSANS(Instrument):
    """
        HFIR SANS instrument description
    """
    def __init__(self, instrument_id="BIOSANS") :
        self._NAME = instrument_id
        super(HFIRSANS, self).__init__()
        
        ## Number of detector pixels in X
        self.nx_pixels = int(self.definition.getNumberParameter("number-of-x-pixels")[0])
        ## Number of detector pixels in Y
        self.ny_pixels = int(self.definition.getNumberParameter("number-of-y-pixels")[0])
        ## Number of counters. This is the number of detector channels (spectra) before the
        # data channels start
        self.nMonitors = int(self.definition.getNumberParameter("number-of-monitors")[0])
        ## Pixel size in mm
        self.pixel_size_x = self.definition.getNumberParameter("x-pixel-size")[0]
        self.pixel_size_y = self.definition.getNumberParameter("y-pixel-size")[0]
        ## Detector name
        self.detector_ID = self.definition.getStringParameter("detector-name")[0]
        ## Wavelength value to be used instead of value read from data file
        self.wavelength = None
        self.wavelength_spread = None

    def set_wavelength(self, wavelength=None, spread=None):
        """
            Sets the wavelength. If the wavelength is set to something other 
            than None, that value will take precedence over any value read in 
            the data files.
            @param wavelength: value of the wavelength [Angstrom]
            @param spread: wavelength spread [Angstrom]
        """
        self.wavelength = wavelength
        self.wavelength_spread = spread
        
    def get_default_beam_center(self):
        """
            Returns the default beam center position, or the pixel location
            of real-space coordinates (0,0).
        """
        return self.get_pixel_from_coordinate(0, 0)

        
    def get_pixel_from_coordinate(self, x=0, y=0):
        """
            Returns the pixel coordinates corresponding to the
            given real-space position.
            
            This assumes that the center of the detector is aligned 
            with the beam. An additional offset may need to be applied
            
            @param x: real-space x coordinate [m]
            @param y: real-space y coordinate [m]
        """
        return [x/self.pixel_size_x*1000.0 + self.nx_pixels/2.0-0.5,
                y/self.pixel_size_y*1000.0 + self.ny_pixels/2.0-0.5]
    
    def get_coordinate_from_pixel(self, x=0, y=0):
        """
            Returns the real-space coordinates corresponding to the
            given pixel coordinates [m].
            
            This assumes that the center of the detector is aligned 
            with the beam. An additional offset may need to be applied
            
            @param x: pixel x coordinate
            @param y: pixel y coordinate
        """
        return [(x-self.nx_pixels/2.0+0.5) * self.pixel_size_x/1000.0,
                (y-self.ny_pixels/2.0+0.5) * self.pixel_size_y/1000.0]
    
    def get_masked_pixels(self, nx_low, nx_high, ny_low, ny_high):
        """
            Generate a list of masked pixels.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector
            
            TODO: hide this in an instrument-specific mask implementation
        """
        masked_x = range(0, nx_low)
        masked_x.extend(range(self.nx_pixels-nx_high, self.nx_pixels))

        masked_y = range(0, ny_low)
        masked_y.extend(range(self.ny_pixels-ny_high, self.ny_pixels))
        
        masked_pts = []
        for y in masked_y:
            masked_pts.extend([ [y,x] for x in range(self.nx_pixels) ])
        for x in masked_x:
            masked_pts.extend([ [y,x] for y in range(ny_low, self.ny_pixels-ny_high) ])
        
        return masked_pts
        
    def get_detector_from_pixel(self, pixel_list):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
        """
        return [ 1000000 + p[0]*1000 + p[1] for p in pixel_list ]
        
        
    def get_aperture_distance(self, workspace):
        """
            Return the aperture distance
        """
        try:
            nguides = MantidFramework.mtd[workspace].getRun().getProperty("number-of-guides").value
            apertures_lst = MantidFramework.mtd[workspace].getInstrument().getStringParameter("aperture-distances")[0]
            apertures = apertures_lst.split(',')
            # Note that they are in reverse order, the first item is for 8 guides
            # and the last item is for 0 guide.
            index = 8-nguides
            return float(apertures[index])
        except:
            raise RuntimeError, "Could not find the for %s\n  %s" % (workspace, sys.exc_value)
