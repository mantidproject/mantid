import unittest
import os
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *
from testhelpers import run_algorithm

class MaskWorkspaceToCalFileTest(unittest.TestCase):

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

    def do_test_cal_file(self, masked_workspace, should_invert, expected_masking_identifier, expected_not_masking_identifier, masking_edge):
        cal_filename = 'wish_masking_system_test_temp.cal'
        cal_file_alg = run_algorithm('MaskWorkspaceToCalFile',InputWorkspace=masked_workspace.name(), OutputFile=cal_filename, Invert=should_invert, rethrow=True)

        cal_file_full_path = str(cal_file_alg.getPropertyValue('OutputFile'))
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

    # Test creating a cal file from a masked MatrixWorkspace directly
    def test_calfile_from_masked_workspace(self):
        run_algorithm('Load', Filename='LOQ49886.nxs', OutputWorkspace='LOQ49886', rethrow=True)
        run_algorithm('MaskDetectors', Workspace='LOQ49886',WorkspaceIndexList='0-700', MaskedWorkspace='LOQ49886', rethrow=True)
        masked_workspace = mtd['LOQ49886']
        should_invert = False
        masking_identifier = 0
        not_masking_identifier = 1
        self.do_test_cal_file(masked_workspace, should_invert, masking_identifier, not_masking_identifier, 700)

    # Test creating a cal file from a MaskWorkspace
    def test_calfile_from_extracted_masking_workspace(self):
        run_algorithm('Load', Filename='LOQ49886.nxs', OutputWorkspace='LOQ49886', rethrow=True)
        run_algorithm('MaskDetectors', Workspace='LOQ49886',WorkspaceIndexList='0-700', MaskedWorkspace='LOQ49886', rethrow=True)
        run_algorithm('ExtractMask', InputWorkspace='LOQ49886', OutputWorkspace='ExtractedWorkspace', rethrow=True)
        extracted_workspace = mtd['ExtractedWorkspace']
        should_invert = False
        masking_identifier = 0
        not_masking_identifier = 1
        self.do_test_cal_file(extracted_workspace, should_invert, masking_identifier, not_masking_identifier, 700)

    # Test creating a cal file from a MatrixWorkspace directly with masking inverted
    def test_calfile_from_masked_workspace_inverse(self):
        run_algorithm('Load', Filename='LOQ49886.nxs', OutputWorkspace='LOQ49886', rethrow=True)
        run_algorithm('MaskDetectors', Workspace='LOQ49886',WorkspaceIndexList='0-700', MaskedWorkspace='LOQ49886', rethrow=True)
        masked_workspace = mtd['LOQ49886']
        should_invert = True
        masking_identifier = 1
        not_masking_identifier = 0
        self.do_test_cal_file(masked_workspace, should_invert, masking_identifier, not_masking_identifier, 700)

    # Test creating a cal file from a MaskWorkspace with masking inverted
    def test_calfile_from_extracted_masking_workspace_inverse(self):
        run_algorithm('Load', Filename='LOQ49886.nxs', OutputWorkspace='LOQ49886', rethrow=True)
        run_algorithm('MaskDetectors', Workspace='LOQ49886',WorkspaceIndexList='0-700', MaskedWorkspace='LOQ49886', rethrow=True)
        run_algorithm('ExtractMask', InputWorkspace='LOQ49886', OutputWorkspace='ExtractedWorkspace', rethrow=True)
        extracted_workspace = mtd['ExtractedWorkspace']
        should_invert = True
        masking_identifier = 1
        not_masking_identifier = 0
        self.do_test_cal_file(extracted_workspace, should_invert, masking_identifier, not_masking_identifier, 700)



if __name__ == '__main__':
    unittest.main()