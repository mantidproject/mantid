"""
    Classes for each reduction step. Those are kept separately 
    from the the interface class so that the HFIRReduction class could 
    be used independently of the interface implementation
"""


class BeamFinder(object):
    """
        Small class to hold the state of the interface
    """
    # Beam finder
    x_position = 0
    y_position = 0
    use_finder = False
    beam_file = ''
    beam_radius = 3.0
    use_direct_beam = True
    
    def __str__(self):
        """
            Representation as a Mantid script
        """
        script = ""
        if not self.use_finder:
            script += "SetBeamCenter(%g, %g)\n" % (self.x_position, self.y_position) 
        else:
            if self.use_direct_beam:
                script += "DirectBeamCenter(\"%s\")\n" % self.beam_file
            else:
                script += "ScatteringBeamCenter(\"%s\", %g)\n" % (self.beam_file, self.beam_radius)
        return script

    def apply(self, reducer):
        """
            The equivalent of the command line implementation, directly
            applied to a SANSReducer object
            @param reducer: SANSReducer object
        """
        return NotImplemeted