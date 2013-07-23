"""
    Instrument class for EQSANS reduction
"""
from reduction import Instrument
import mantid

class EQSANS(Instrument):
    """
        HFIR SANS instrument description
    """
    _NAME = "EQ-SANS"
    
    def __init__(self) :
        # We skip the base class initialization because we don't need
        # to load the instrument description until later 
        
        ## Detector name
        self.detector_ID = "detector1"
        # Slit to source distance in mm for the three slit wheels
        self.slit_to_source = [10080, 11156, 12150]

    def get_incident_mon(self, workspace, option):
        return -1
    
    def get_default_beam_center(self, workspace=None):
        """
            Returns the default beam center position, or the pixel location
            of real-space coordinates (0,0).
        """
        return self.get_pixel_from_coordinate(0, 0, workspace)

    def get_pixel_from_coordinate(self, x, y, workspace=None):
        """
            Returns the pixel coordinates corresponding to the
            given real-space position.
            
            This assumes that the center of the detector is aligned 
            with the beam. An additional offset may need to be applied
            
            @param x: real-space x coordinate [m]
            @param y: real-space y coordinate [m]
            @param workspace: the pixel number and size info will be taken from the workspace
        """
        nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = self._get_pixel_info(workspace)
        
        return [-x/pixel_size_x*1000.0 + nx_pixels/2.0-0.5,
                y/pixel_size_y*1000.0 + ny_pixels/2.0-0.5]
    
    def get_coordinate_from_pixel(self, x, y, workspace=None):
        """
            Returns the real-space coordinates corresponding to the
            given pixel coordinates [m].
            
            This assumes that the center of the detector is aligned 
            with the beam. An additional offset may need to be applied
            
            @param x: pixel x coordinate
            @param y: pixel y coordinate
            @param workspace: the pixel number and size info will be taken from the workspace
        """
        nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = self._get_pixel_info(workspace)

        return [(nx_pixels/2.0-0.5-x) * pixel_size_x/1000.0,
                (y-ny_pixels/2.0+0.5) * pixel_size_y/1000.0]
            
    def get_masked_pixels(self, nx_low, nx_high, ny_low, ny_high, workspace=None):
        """
            Generate a list of masked pixels.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector
            @param workspace: the pixel number and size info will be taken from the workspace
        """
        nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = self._get_pixel_info(workspace)

        masked_x = range(0, nx_low)
        masked_x.extend(range(nx_pixels-nx_high, nx_pixels))

        masked_y = range(0, ny_low)
        masked_y.extend(range(ny_pixels-ny_high, ny_pixels))
        
        masked_pts = []
        for y in masked_y:
            masked_pts.extend([ [x,y] for x in range(nx_pixels) ])
        for x in masked_x:
            masked_pts.extend([ [x,y] for y in range(ny_low, ny_pixels-ny_high) ])
        
        return masked_pts
        
    def _get_pixel_info(self, workspace):
        """
            Get the pixel size and number of pixels from the workspace
            @param workspace: workspace to extract the pixel information from
        """
        ## Number of detector pixels in X
        nx_pixels = int(mantid.mtd[workspace].getInstrument().getNumberParameter("number-of-x-pixels")[0])
        ## Number of detector pixels in Y
        ny_pixels = int(mantid.mtd[workspace].getInstrument().getNumberParameter("number-of-y-pixels")[0])
        ## Pixel size in mm
        pixel_size_x = mantid.mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mantid.mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]

        return nx_pixels, ny_pixels, pixel_size_x, pixel_size_y
                
    def get_detector_from_pixel(self, pixel_list, workspace):
        """
            Returns a list of detector IDs from a list of [x,y] pixels,
            where the pixel coordinates are in pixel units.
            @param workspace: the pixel number and size info will be taken from the workspace
        """
        nx_pixels, ny_pixels, pixel_size_x, pixel_size_y = self._get_pixel_info(workspace)
        return [ ny_pixels*p[0] + p[1] for p in pixel_list ]
        
