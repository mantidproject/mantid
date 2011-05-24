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
    
    # Use TOF cuts from configuration file
    use_config_cutoff = True
    low_TOF_cut = 0.0
    high_TOF_cut = 0.0
    # Correct for flight path at larger angle
    correct_for_flight_path = True

    def __init__(self):
        super(ReductionOptions, self).__init__()
        self.reset()
        
    def reset(self):
        super(ReductionOptions, self).reset()
        self.nx_pixels = ReductionOptions.nx_pixels
        self.ny_pixels = ReductionOptions.ny_pixels
        self.pixel_size = ReductionOptions.pixel_size
        self.detector_offset = ReductionOptions.detector_offset

        self.use_config_cutoff = ReductionOptions.use_config_cutoff
        self.low_TOF_cut = ReductionOptions.low_TOF_cut
        self.high_TOF_cut = ReductionOptions.high_TOF_cut
        self.correct_for_flight_path = ReductionOptions.correct_for_flight_path

    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = super(ReductionOptions, self).to_script()
        
        # TOF cutoff
        if self.use_config_cutoff:
            script += "UseConfigTOFTailsCutoff(True)\n"
        else:
            script += "UseConfigTOFTailsCutoff(False)\n"
            if self.low_TOF_cut>0 or self.high_TOF_cut>0:
                script += "SetTOFTailsCutoff(%g, %g)\n" % (self.low_TOF_cut, self.high_TOF_cut)
                
        # Flight path correction
        script += "PerformFlightPathCorrection(%s)\n" % self.correct_for_flight_path
        
        return script
            
            
