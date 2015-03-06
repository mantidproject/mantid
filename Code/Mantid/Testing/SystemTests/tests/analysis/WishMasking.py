"""
Tests masking functionality specific to WISH. Working masking behaviour is critical in general, but is heavily used on WISH. 
- Email Pascal Manuel @ ISIS if things break here and let him know how his scripts may need to be modified.
"""

import stresstesting
import os
from mantid.simpleapi import *

class WishMasking(stresstesting.MantidStressTest):
	
	# Utility function will return the masking corresponding to a workspace index from a cal file.
	def get_masking_for_index(self, cal_file, requested_index):
		while True:
			line = cal_file.readline()
			if line == "":
				raise LookupError
			line_contents = line.split()
			try:
				index = int(line_contents[0].strip()) 
				select = int(line_contents[3].strip())
				group = int(line_contents[4].strip())
				if(index == requested_index):
					return select
			except ValueError:
				continue
	
        # Tests that the cal file is being created in the expected way.	
	#  1) Uses the masks to create a cal file
	#  2) Read the cal file
	#  3) Use the known masking boundaries to determine whether the cal file has been created propertly accoring to the function inputs.
	def do_test_cal_file(self, masked_workspace, should_invert, expected_masking_identifier, expected_not_masking_identifier, masking_edge):
		
		cal_filename = 'wish_masking_system_test_temp.cal'
                cal_file_full_path = os.path.join(config['defaultsave.directory'],cal_filename)
		MaskWorkspaceToCalFile(InputWorkspace=masked_workspace, OutputFile=cal_file_full_path, Invert=should_invert)
		file = open(cal_file_full_path, 'r')
		try:
			mask_boundary_inside = self.get_masking_for_index(file, masking_edge)
			mask_boundary_outside = self.get_masking_for_index(file, masking_edge+1)
			self.assertTrue(mask_boundary_inside == expected_masking_identifier)
			self.assertTrue(mask_boundary_outside == expected_not_masking_identifier)
		except LookupError:
			print "Could not find the requested index"
			self.assertTrue(False)
		finally:
			file.close()
			os.remove(cal_file_full_path)
	
	def requiredMemoryMB(self):
		return 2000
	
	def runTest(self):
		Load(Filename='WISH00016748.raw',OutputWorkspace='wish_ws')
		ws = mtd['wish_ws']
		MaskDetectors(Workspace=ws, WorkspaceIndexList='0,1,2,3,4,5,6,7,8,9')
		
		# We just masked all detectors up to index == 9
		masking_edge = 9 
		
		# Test the 'isMasked' property on the detectors of the original workspace
		self.assertTrue( ws.getDetector(masking_edge).isMasked() )
		self.assertTrue( not ws.getDetector(masking_edge + 1).isMasked() )
		
		# Extract a masking workspace
		ExtractMask( InputWorkspace=ws, OutputWorkspace='masking_wish_workspace' )
		mask_ws =  mtd['masking_wish_workspace']
		
		## COMPLETE TESTS: These following are the tests that should pass when everything works. See below for reasons why. 
		
		# Test the 'isMasked' property on the detectors of the masked workspace
		# The following tests have been added even though they are broken because extracted workspaces currently do not preserve the Masking flags (buty they SHOULD!). Hopefully the broken functionality will be fixed and I can enable them.
		#self.assertTrue( mask_ws.getDetector(masking_edge).isMasked() )
		#self.assertTrue( not mask_ws.getDetector(masking_edge + 1).isMasked() )
		
		# Save masking
		mask_file = 'wish_masking_system_test_mask_file_temp.xml'
		SaveMask(InputWorkspace=mask_ws,OutputFile=mask_file)
                mask_file_path = os.path.join(config['defaultsave.directory'], mask_file)
		# Check the mask file was created.
		self.assertTrue(os.path.isfile(mask_file_path)) 
                # ... and has the correct contents
                masking_xml = open(mask_file_path, 'r')
                found_correct_ids = False
                for line in masking_xml:
                        if "<detids>1-5,1100000-1100019</detids>" in line:
                                found_correct_ids = True
                masking_xml.close()
                self.assertTrue(found_correct_ids)
                os.remove(mask_file_path)
		
		## END COMPLETE TESTS 
		
		## CHARACTERISATION TESTS: These tests characterise the current breakage of the masking code.
		## I've included these false-positives as a testing strategy because it will flag up that the functionality has been fixed when these tests start failing (we can then test the right thing, see above)
		
		# Testing that the isMasking is the same on both sides of the masking boundary. If things were working properly the following would not pass!
		self.assertTrue( mask_ws.getDetector(masking_edge).isMasked() == mask_ws.getDetector(masking_edge + 1).isMasked() )
		## END CHARACTERISATION TESTS 
		
		#Test creation with normal masking
		invert_masking = False;
		self.do_test_cal_file(ws, invert_masking, 0, 1, masking_edge)
		
		#Test with masking inversed, because that is a real schenario too.
		invert_masking = True;
		self.do_test_cal_file(ws, invert_masking, 1, 0, masking_edge)
		
		#Test merge cal files
		master_cal_file_name = 'master.cal'
		update_cal_file_name =  'update.cal'
		merged_cal_file_name = 'merged.cal'
                save_path = config['defaultsave.directory']
		master_cal_file_path = os.path.join(save_path,master_cal_file_name)
		update_cal_file_path = os.path.join(save_path,update_cal_file_name)
		merged_cal_file_path = os.path.join(save_path,merged_cal_file_name)
		
		try:
			MaskWorkspaceToCalFile(InputWorkspace=ws, OutputFile=master_cal_file_name, Invert=False)
			MaskWorkspaceToCalFile(InputWorkspace=ws, OutputFile=update_cal_file_name, Invert=True)
			
			MergeCalFiles(UpdateFile=update_cal_file_path, MasterFile=master_cal_file_path, 
                                      OutputFile=merged_cal_file_name, MergeSelections=True)
                			
			update_cal_file = open(update_cal_file_path, 'r')
			merged_cal_file = open(merged_cal_file_path, 'r')
			
			merged_mask_boundary_inside = self.get_masking_for_index(merged_cal_file, masking_edge)
			merged_mask_boundary_outside = self.get_masking_for_index(merged_cal_file, masking_edge+1)
			update_mask_boundary_inside = self.get_masking_for_index(update_cal_file, masking_edge)
			update_mask_boundary_outside = self.get_masking_for_index(update_cal_file, masking_edge+1)
			
			#Test that the merged output cal file has actually taken the masking from the update file.
			self.assertTrue(merged_mask_boundary_inside != merged_mask_boundary_outside)
			self.assertTrue(merged_mask_boundary_inside == update_mask_boundary_inside)
			self.assertTrue(merged_mask_boundary_outside == update_mask_boundary_outside)
			
		finally:
			#clean up no matter what.
			merged_cal_file.close()
			update_cal_file.close()
			os.remove(master_cal_file_path)
			os.remove(update_cal_file_path)
			os.remove(merged_cal_file_path)
		
	def doValidate(self):
		return True;
