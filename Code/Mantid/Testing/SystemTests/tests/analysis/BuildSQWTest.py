#pylint: disable=invalid-name
"""
    Defines a system test for converting a set of reduced direct inelastic data
    to a single SQW file.

    The test requires as input the set of reduced files, which are ~16Gb along with
    the result file that is ~30Gb. The files are not included with the standard
    repository & required to be accessible from any machine that wishes to run the test.
"""
import stresstesting
from mantid.simpleapi import *

import os

# allow for multiple locations
FILE_LOCATIONS = ["/isis/mantid/localtestdata/"]#,"d:/Data/MantidSystemTests/BigData/Dropbox/LoadSQW"]

class BuildSQWTest(stresstesting.MantidStressTest):

    _startrun = 15058
    _endrun = 15178
    _input_data = []
    _input_location = None
    _created_files = []

    def __init__(self):
        super(BuildSQWTest, self).__init__()
        prefix = "MAP"
        ext = ".nxspe"
        # MAP*.nxspe data files
        self._input_data = ["%s%d%s" % (prefix,n,ext) for n in range(self._startrun,self._endrun+1)]

    def skipTests(self):
        def check_dir(loc):
            for filename in self._input_data:
                path = os.path.join(loc, filename)
                if not os.path.exists(path):
                    return False
            return True
        # end nested function

        all_found = False
        for location in FILE_LOCATIONS:
            if check_dir(location):
                self._input_location = location
                all_found = True
                break

        skip = (not all_found)
        return skip

    def runTest(self):
        conversion_params = {}
        conversion_params['QDimensions'] = 'Q3D'
        conversion_params['dEAnalysisMode'] = 'Direct'
        conversion_params['Q3DFrames'] = 'HKL'
        conversion_params['QConversionScales'] = 'HKL'
        conversion_params['PreprocDetectorsWS'] = '_preprocessed_detectors'
        conversion_params['MinValues'] = '-7,-7,-7.,-72.0'
        conversion_params['MaxValues'] = '7.,7.,7.,382.0'
        conversion_params['SplitInto'] = 50
        conversion_params['MaxRecursionDepth'] = 1
        conversion_params['MinRecursionDepth'] = 1

        self._created_files = []
        for source in self._input_data:
            source_path = os.path.join(self._input_location, source)
            target = os.path.join(config["defaultsave.directory"], "MD" + source.rstrip(".nxspe") + ".nxs")
            # Make sure the target doesn't exist from a previous test
            if os.path.exists(target):
                os.remove(target)

            print "Converting '%s' to '%s' " % (source_path,target)
            _cur_spe_ws = LoadNXSPE(Filename=source_path)
            SetUB(Workspace=_cur_spe_ws,a='2.87',b='2.87',c='2.87')
            # rotated by proper number of degrees around axis Y
            # sample log Psi should already be there
            SetGoniometer(Workspace=_cur_spe_ws,Axis0='Psi,0,1,0,1')

            conversion_params['InputWorkspace'] = _cur_spe_ws
            _cur_md_ws = ConvertToMD(**conversion_params)

            SaveMD(InputWorkspace=_cur_md_ws,Filename=target)
            self._created_files.append(target)
            DeleteWorkspace(_cur_spe_ws)
            DeleteWorkspace(_cur_md_ws)
        # end conversion loop

        # Do the final merge
        sqw_file = os.path.join(config["defaultsave.directory"],"BuildSQWTestCurrent.nxs")
        dummy_finalSQW = MergeMDFiles(",".join(self._created_files),OutputFilename=sqw_file,Parallel='0')
        self._created_files.append(sqw_file)

    def validate(self):
        # LoadMD is unable to load the merged output file. See ticket #8480.
        # At the moment this test is useful for benchmarking the conversion so it exists purely
        # for timing purposes until #8480 is fixed
        return True

    def cleanup(self):
        for filename in self._created_files:
            try:
                os.remove(filename)
            except OSError:
                mantid.logger.warning("Unable to remove created file '%s'" % filename)

class LoadSQW_FileBasedTest(BuildSQWTest):
    """ The test checks loading MD workspace from SQW file when target file is file based"""

    def __init__(self):
        super(LoadSQW_FileBasedTest, self).__init__()
        self._input_data = ["Test22meV2f.sqw","Test22meVMD.nxs"]

    def runTest(self):

        MDws_file = os.path.join(config["defaultsave.directory"],"LoadSQWTestFileBased.nxs")
        sqw_file = os.path.join(self._input_location,self._input_data[0])

        dummy_wsMD=LoadSQW(Filename=sqw_file, OutputFilename=MDws_file)

        self._created_files=MDws_file


    def validate(self):
        """Compare file-based MD files """
        ref_file = os.path.join(self._input_location, self._input_data[1])
        Reference=LoadMD(Filename=ref_file, FileBackEnd=True, Memory=100)
        rez = CompareMDWorkspaces(Workspace1="wsMD",Workspace2=Reference,Tolerance=1.e-5,CheckEvents=False,IgnoreBoxID=False)

        DeleteWorkspace("dummy_wsMD")

        return rez[0]

class LoadSQW_MemBasedTest(BuildSQWTest):
    """ The test checks loading MD workspace from SQW file when target file is file based"""

    def __init__(self):
        super(LoadSQW_MemBasedTest, self).__init__()
        self._input_data = ["Test22meV2f.sqw","Test22meVMD.nxs"]

    def runTest(self):

        sqw_file = os.path.join(self._input_location,self._input_data[0])

        dummy_wsMD=LoadSQW(Filename=sqw_file)

        self._created_files=[]


    def validate(self):
        """Compare memory-based vs file based MD workspaces """
        ref_file = os.path.join(self._input_location, self._input_data[1])
        Reference=LoadMD(Filename=ref_file, FileBackEnd=True, Memory=100)
        rez = CompareMDWorkspaces(Workspace1="wsMD",Workspace2=Reference,Tolerance=1.e-5,CheckEvents=False,IgnoreBoxID=False)

        DeleteWorkspace("dummy_wsMD")

        return rez[0]
