"""
    General options for EQSANS reduction
"""
from hfir_options_script import ReductionOptions as BaseOptions

class ReductionOptions(BaseOptions):
    instrument_name = "EQSANS"
    nx_pixels = None
    ny_pixels = None
    pixel_size = None
    detector_offset = 0.0

    def __init__(self):
        super(ReductionOptions, self).__init__()
        self.reset()
        
    def reset(self):
        super(ReductionOptions, self).reset()
        self.nx_pixels = ReductionOptions.nx_pixels
        self.ny_pixels = ReductionOptions.ny_pixels
        self.pixel_size = ReductionOptions.pixel_size
        self.detector_offset = ReductionOptions.detector_offset
    

