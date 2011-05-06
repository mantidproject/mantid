"""
    Sample data options for EQSANS reduction
"""
from reduction_gui.reduction.scripter import BaseScriptElement
from reduction_gui.reduction.sans.hfir_sample_script import SampleData as BaseSampleData

class SampleData(BaseSampleData):

    class BeamHole(BaseScriptElement):        
        def to_script(self):
            """
                Generate reduction script
            """
            return "BeamStopTransmission()\n"
            
        def to_xml(self):
            """
                Create XML from the current data.
            """
            return "  <BeamHole/>\n"

        def find(self, dom):
            """
                Return True if we find a tag belonging 
                to this class
            """
            element_list = dom.getElementsByTagName("BeamHole")
            return len(element_list)>0
        
    calculation_method = BeamHole()
    # Option list
    option_list = [BaseSampleData.DirectBeam, BeamHole]

    def reset(self):
        """
            Reset state
        """
        super(SampleData, self).reset()
        self.calculation_method = SampleData.calculation_method
    

    