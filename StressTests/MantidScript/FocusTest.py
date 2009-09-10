import stresstesting
from mantidsimple import *

class FocusTest(stresstesting.MantidStressTest):
    
    def runTest(self):
        '''A simple test of the focussing chain on our test GEM data'''
        LoadRaw('GEM38370.raw', 'GEM')
        AlignDetectors('GEM', 'GEM', 'offsets_2006_cycle064.cal')
        DiffractionFocussing('GEM', 'GEM', 'offsets_2006_cycle064.cal')
        
    def maxIterations(self):
        return 5
       

