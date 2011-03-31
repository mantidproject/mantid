"""
    Unit tests for SANS reduction
"""
# Add the Interface directory to the python path
import sys
sys.path.append('..')

from reduction.hfir_reduction_steps import *
from reduction.hfir_reduction import ReductionScripter
import sys
import unittest

class TestSteps(unittest.TestCase):
    
    def setUp(self):
        pass

    def test_scripter(self):
        s = ReductionScripter()
        s.from_xml("test.xml")
        self.assertEqual(s.instrument.name, 'BIOSANS-TEST')
        self.assertEqual(s.beam_finder.beam_file, 'some file')
        
        s.to_xml("test_output.xml")
        m = ReductionScripter()
        m.from_xml("test.xml")
        self.assertEqual(m.instrument.name, 'BIOSANS-TEST')
        self.assertEqual(m.beam_finder.beam_file, 'some file')
        
    def test_beam_finder(self):
        b = BeamFinder()
        b.x_position = 1.0
        b.y_position = 2.0
        b.use_finder = True
        b.beam_file = 'some file'
        b.beam_radius = 3.0
        b.use_direct_beam = False
        xml_str = b.to_xml()
        
        c = BeamFinder()
        c.from_xml(xml_str)
        
        self.assertEqual(c.x_position, b.x_position)
        self.assertEqual(c.y_position, b.y_position)
        self.assertEqual(c.use_finder, b.use_finder)
        self.assertEqual(c.beam_file, b.beam_file)
        self.assertEqual(c.beam_radius, b.beam_radius)
        self.assertEqual(c.use_direct_beam, b.use_direct_beam)

        
        
if __name__ == '__main__':
    unittest.main()
    
    