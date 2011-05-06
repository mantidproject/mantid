"""
    Sample data options for EQSANS reduction
"""
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.hfir_background_script import Background as BaseBackground
from reduction_gui.reduction.sans.eqsans_sample_script import SampleData

class Background(BaseBackground):

    class DirectBeam(SampleData.DirectBeam):
        def __init__(self, state=None):
            SampleData.DirectBeam.__init__(self)
            if state is not None:
                self.sample_file = state.sample_file
                self.direct_beam = state.direct_beam
                self.beam_radius = state.beam_radius

        def to_script(self):
            """
                Generate reduction script
            """
            if len(str(self.sample_file).strip())==0 or len(str(self.direct_beam).strip())==0:
                raise RuntimeError, "Direct beam method for background transmission was selected but was selected but all the appropriate data files were not entered."    
            
            return "BckDirectBeamTransmission(\"%s\", \"%s\", beam_radius=%g)\n" % \
            (self.sample_file, self.direct_beam, self.beam_radius)

    class BeamHole(SampleData.BeamHole):        
        def __init__(self, state=None):
            SampleData.BeamHole.__init__(self)

        def to_script(self):
            """
                Generate reduction script
            """
            return "BckBeamStopTransmission()\n"
            
    trans_calculation_method = BeamHole()
    # Option list
    option_list = [DirectBeam, BeamHole]

    def reset(self):
        """
            Reset state
        """
        super(Background, self).reset()
        self.trans_calculation_method = Background.trans_calculation_method
    

    