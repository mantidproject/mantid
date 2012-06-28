import unittest

from mantid.kernel import *
from mantid.api import *
from testhelpers import run_algorithm

class Stitch1DTest(unittest.TestCase):

    __good_workspace_name = None
    __bad_type_of_workspace_name = None
    __three_dim_workspace_name = None
    __integrated_two_dim_workspace_name = None
    __unintegrated_two_dim_workspace_name = None

    def setUp(self):
        if self.__good_workspace_name == None:
        
            # Create a workspace of the wrong type
            bad_md_workspace_alg = run_algorithm("CreateMDWorkspace", Extents='0,1',Names='A',Units='U',OutputWorkspace='Stitch1D_test_workspace_1')
            self.__bad_type_of_workspace_name = bad_md_workspace_alg.getPropertyValue("OutputWorkspace")
            
            # Create a workspace that is of the right type and shape
            good_workspace_alg = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2',ErrorInput='1,2',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='2,1',Names='A,B',Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_2')
            self.__good_workspace_name = good_workspace_alg.getPropertyValue("OutputWorkspace")
            
            # Create a workspace that is of the right type, but the wrong shape
            three_dim_alg = run_algorithm("CreateMDHistoWorkspace",SignalInput='1',ErrorInput='1',Dimensionality='3',Extents='-1,1,-1,1,-1,1',NumberOfBins='1,1,1',Names='A,B,C',Units='U1,U2,U3',OutputWorkspace='Stitch1D_test_workspace_3')
            self.__three_dim_workspace_name = three_dim_alg.getPropertyValue("OutputWorkspace")
            
            # Create a workspace that is of the right type and shape, but wrong size, with 1 bin in each dimension (completely integrated).
            integrated_two_dim_alg = run_algorithm("CreateMDHistoWorkspace",SignalInput='1',ErrorInput='1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='1,1',Names='A,B',Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_4')
            self.__integrated_two_dim_workspace_name = integrated_two_dim_alg.getPropertyValue("OutputWorkspace")
            
            # Create a workspace that is of the right type and shape, but wrong size, with more than one bin in both dimensions (completely unintegrated).
            unintegrated_two_dim_alg = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,1,1,1',ErrorInput='1,1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='2,2',Names='A,B',Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_5')
            self.__unintegrated_two_dim_workspace_name = unintegrated_two_dim_alg.getPropertyValue("OutputWorkspace")
            
    def tearDown(self):
        mtd.remove(self.__good_workspace_name)
        mtd.remove(self.__bad_type_of_workspace_name)
        mtd.remove(self.__three_dim_workspace_name)
        mtd.remove(self.__integrated_two_dim_workspace_name)
        mtd.remove(self.__unintegrated_two_dim_workspace_name)
        
    def test_does_not_accept_mdeventworkspaces_for_ws1(self):
        passed = False
        
        try:
            # Should throw value error for wrong type of workspace1.
            run_algorithm("Stitch1D", Workspace1=self.__bad_type_of_workspace_name,Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
            
    def test_does_not_accept_mdeventworkspaces_for_ws2(self):
        passed = False
        try:
            # Should throw value error for wrong type of workspace1.
            run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name,Workspace2=self.__bad_type_of_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
    
    def test_ws1_with_three_input_dimensions_throws(self): 
        passed = False
        try:
            # Should throw with ws1 three-dimensional
            run_algorithm("Stitch1D", Workspace1=self.__three_dim_workspace_name,Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed)
        
    def test_ws2_with_three_input_dimensions_throws(self): 
        passed = False
        try:
            # Should throw with ws2 three-dimensional
            run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name ,Workspace2=self.__three_dim_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed)
    
    def test_ws1_with_two_integrated_dimensions_throws(self):
        passed = False
        try:
            # Should throw with ws1 fully integrated
            run_algorithm("Stitch1D", Workspace1=self.__integrated_two_dim_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed)  
        
    def test_ws2_with_two_integrated_dimensions_throws(self):
        passed = False
        try:
            # Should throw with ws2 fully integrated
            run_algorithm("Stitch1D", Workspace2=self.__integrated_two_dim_workspace_name, Workspace1=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed)  
 
    def test_ws1_with_two_non_integrated_dimensions_throws(self): 
        passed = False
        try:
            # Should throw with ws1 fully unintegrated
            run_algorithm("Stitch1D", Workspace1=self.__unintegrated_two_dim_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed) 
    
    def test_ws2_with_two_non_integrated_dimensions_throws(self): 
        passed = False
        try:
            # Should throw with ws2 fully unintegrated
            run_algorithm("Stitch1D", Workspace2=self.__unintegrated_two_dim_workspace_name, Workspace1=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        self.assertTrue(passed) 
    
    def test_ws1_and_ws2_have_different_binning_throws(self):
        # Create Workspace with 2 bins
        alg_A = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2',ErrorInput='1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='2,1',Names='A,B',Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_A')
        # Create Workspace with 3 bins
        alg_B = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2,3',ErrorInput='1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_B')
        
        a = alg_A.getPropertyValue("OutputWorkspace")
        b = alg_B.getPropertyValue("OutputWorkspace")
        
        passed = False
        try:
            # Different binning in the unintegrated dimension between Workspace1 and Workspace2
            run_algorithm("Stitch1D", Workspace1=a, Workspace2=b,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        finally:
            mtd.remove(a)
            mtd.remove(b)
            self.assertTrue(passed) 
            
    def __do_test_ws1_and_ws2_have_different_dimension_names_throws(self, ws1_dim_names, ws2_dim_names):
        # Create Workspace with dim names in ws1_dim_names
        alg_A = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2',ErrorInput='1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='2,1',Names=ws1_dim_names,Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_C')
        # Create Workspace with dim names in ws2_dim_names
        alg_B = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2',ErrorInput='1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='2,1',Names=ws2_dim_names,Units='U1,U2',OutputWorkspace='Stitch1D_test_workspace_D')
        
        a = alg_A.getPropertyValue("OutputWorkspace")
        b = alg_B.getPropertyValue("OutputWorkspace")

        passed = False
        try:
            # Different dimension_names in the unintegrated dimension between Workspace1 and Workspace2
            run_algorithm("Stitch1D", Workspace1=a, Workspace2=b,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.3, child=True) 
        except RuntimeError:
            passed = True
        finally:
            mtd.remove(a)
            mtd.remove(b)
            self.assertTrue(passed) 
    
    def test_ws1_and_ws2_dim1_have_different_dimension_names_throws(self):
        self.__do_test_ws1_and_ws2_have_different_dimension_names_throws(['A1','B1'], ['A2', 'B1'])
        
    def test_ws1_and_ws2_dim2_have_different_dimension_names_throws(self):
        self.__do_test_ws1_and_ws2_have_different_dimension_names_throws(['A1','B1'], ['A2', 'B1'])
     
    def test_start_overlap_too_low(self):
        passed = False
        try:
            alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=-1,EndOverlap=0.3) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
            
    def test_start_overlap_too_high(self):
        passed = False
        try:
            alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=1.001,EndOverlap=0.3) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
            
    def test_end_overlap_too_low(self):
        passed = False
        try:
            alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=-1) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
            
    def test_end_overlap_too_high(self):
        passed = False
        try:
            alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=1.001) 
        except ValueError:
            passed = True
        finally:
            self.assertTrue(passed)
	  	
    def test_end_overlap_equal_to_start_overlap_throws(self):
        passed = False	  	
        try:
            alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0,child=True) 
        except RuntimeError:
            passed = True
        finally:
            self.assertTrue(passed)
            
    def test_calculates_scaling_factor_correctly(self):
        # Signal = 1, 1, 1, but only the middle to the end of the range is marked as overlap, so only 1, 1 used.
        alg_a = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,1,1',ErrorInput='1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal')
        # Signal = 1, 2, 3, but only the middle to the end of the range is marked as overlap, so only 2, 3 used.
        alg_b = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2,3',ErrorInput='1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='rising_signal')
   
        alg = run_algorithm("Stitch1D", Workspace1='flat_signal', Workspace2='rising_signal',OutputWorkspace='converted',StartOverlap=0.5,EndOverlap=1,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        
        b_use_manual_scaling = alg.getProperty("UseManualScaleFactor").value
        self.assertEqual(False, b_use_manual_scaling)
        
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        
         # 1 * (( 1 + 1) / (2 + 3)) = 0.4
        self.assertEqual(0.4, scale_factor)
        
    def test_calculates_scaling_factor_correctly_inverted(self):
        # Signal = 1, 1, 1, but only the middle to the end of the range is marked as overlap, so only 1, 1 used.
        alg_a = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,1,1',ErrorInput='1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal')
        # Signal = 1, 2, 3, but only the middle to the end of the range is marked as overlap, so only 2, 3 used.
        alg_b = run_algorithm("CreateMDHistoWorkspace",SignalInput='1,2,3',ErrorInput='1,1,1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='rising_signal')
   
        alg = run_algorithm("Stitch1D", Workspace1='flat_signal', Workspace2='rising_signal',OutputWorkspace='converted',StartOverlap=0.5,EndOverlap=1,ScaleWorkspace2=False,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        
        # 1 * ((2 + 3)/( 1 + 1)) = 2.5
        self.assertEqual(2.5, scale_factor)
       
        
    def test_manual_scaling_factor(self):
        alg_a = run_algorithm("CreateMDHistoWorkspace",SignalInput='1',ErrorInput='1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal')
        alg_b = run_algorithm("CreateMDHistoWorkspace",SignalInput='1',ErrorInput='1',Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins='3,1',Names='A,B',Units='U1,U2',OutputWorkspace='rising_signal')
        
        expected_manual_scale_factor = 2.2
        
        alg = run_algorithm("Stitch1D", Workspace1='flat_signal', Workspace2='rising_signal',OutputWorkspace='converted',StartOverlap=0.5,EndOverlap=1,ScaleWorkspace2=True,UseManualScaleFactor=True,ManualScaleFactor=expected_manual_scale_factor,rethrow=True) 
        self.assertTrue(alg.isExecuted())
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        
        self.assertEqual(expected_manual_scale_factor, scale_factor)
        
    def test_flat_offsetting_schenario(self):
        errors = [1,1,1,1,1,1,1,1,1,1] # Errors for both input ws1 and ws2
        s1 = [1,1,1,1,1,1,1,1,1,1] # Signal values for ws1
        s2 = [3,3,3,3,3,3,3,3,3,3] # Signal values for ws2
        expected_output_signal =[2,2,2,2,2,4,4,4,4,4]
        expected_output_error_sq = [0.5, 0.5, 0.5, 0.5, 0.5, 2, 2, 2, 2, 2]
        alg_a = run_algorithm("CreateMDHistoWorkspace",SignalInput=s1,ErrorInput=errors,Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins=[len(errors),1],Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal_a',rethrow=True)
        alg_b = run_algorithm("CreateMDHistoWorkspace",SignalInput=s2,ErrorInput=errors,Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins=[len(errors),1],Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal_b',rethrow=True)
        # Specify that overlap goes from start to half way along workspace
        alg = run_algorithm("Stitch1D", Workspace1='flat_signal_a', Workspace2='flat_signal_b',OutputWorkspace='converted',StartOverlap=0.0,EndOverlap=0.5,ScaleWorkspace2=True,rethrow=True)    
        # Verify the calculated output values.
        ws = mtd.retrieve(alg.getPropertyValue("OutputWorkspace"))
        for i in range(0, len(errors)):
            self.assertEqual(expected_output_signal[i], ws.signalAt(i))
            self.assertEqual(expected_output_error_sq[i], ws.errorSquaredAt(i))

    def test_flat_offsetting_schenario_with_scaling(self):
        errors = [1,1,1,1,1,1,1,1,1,1] # Errors for both input ws1 and ws2
        s1 = [1,1,1,1,1,1,1,1,1,1] # Signal values for ws1
        s2 = [3,3,3,3,3,3,3,3,3,3] # Signal values for ws2
        expected_output_signal =[2,2,2,2,2,4,4,4,4,4]
        expected_output_error_sq = [0.8, 0.8, 0.8, 0.8, 0.8, 2, 2, 2, 2, 2]
        alg_a = run_algorithm("CreateMDHistoWorkspace",SignalInput=s1,ErrorInput=errors,Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins=[len(errors),1],Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal_a',rethrow=True)
        alg_b = run_algorithm("CreateMDHistoWorkspace",SignalInput=s2,ErrorInput=errors,Dimensionality='2',Extents='-1,1,-1,1',NumberOfBins=[len(errors),1],Names='A,B',Units='U1,U2',OutputWorkspace='flat_signal_b',rethrow=True)
        # Supply a manual scale factor, this will mean that Workspace 1 is scaled by this amount.
        manual_scale_factor = 2
        # Specify that overlap goes from start to half way along workspace
        alg = run_algorithm("Stitch1D", Workspace1='flat_signal_a', Workspace2='flat_signal_b',OutputWorkspace='converted',StartOverlap=0.0,EndOverlap=0.5,UseManualScaleFactor=True,ManualScaleFactor=manual_scale_factor,rethrow=True)    
        # Verify the calculated output values.
        ws = mtd.retrieve(alg.getPropertyValue("OutputWorkspace"))
        for i in range(0, len(errors)):
            self.assertEqual(round(expected_output_signal[i], 5), round(ws.signalAt(i), 5))
            self.assertEqual(round(expected_output_error_sq[i], 5), round(ws.errorSquaredAt(i),5))#
        # Sanity check that the applied scale factor is also the same as the value instructed.
        scale_factor = float(alg.getPropertyValue("OutScaleFactor"))
        self.assertEqual(manual_scale_factor, scale_factor)

    #def test_does_something(self):
        # Algorithm isn't complete at this point, but we need to have one success case to verify that all the previous failure cases are genuine failures (i.e. there is a way to get the algorithm to run properly) 
        #alg = run_algorithm("Stitch1D", Workspace1=self.__good_workspace_name, Workspace2=self.__good_workspace_name,OutputWorkspace='converted',StartOverlap=0,EndOverlap=0.5,rethrow=True) 
        #self.assertTrue(alg.isExecuted())
        #scale_factor = alg.getPropertyValue("AppliedScaleFactor")
        
if __name__ == '__main__':
    unittest.main()