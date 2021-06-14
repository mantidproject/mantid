# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name,attribute-defined-outside-init
from pathlib import Path
import json
import systemtesting
import tempfile
from mantid.simpleapi import CreateGroupingWorkspace, RenameWorkspace, SNAPReduce
import os


def _skip_test():
    """Helper function to determine if we run the test"""
    import platform

    # Only runs on RHEL6 at the moment
    return "Linux" not in platform.platform()


def getSaveDir():
    """determine where to save - the current working directory"""
    return os.path.abspath(os.path.curdir)


def do_cleanup():
    Files = [os.path.join('d_spacing', 'SNAP_34172_2_4_Grouping.dat'),
             os.path.join('gsas', 'SNAP_34172_2_4_Grouping.gsa'),
             os.path.join('fullprof', 'SNAP_34172_2_4_Grouping-0.dat'),
             os.path.join('fullprof', 'SNAP_34172_2_4_Grouping-1.dat'),
             os.path.join('nexus', 'SNAP_34172_2_4_Grouping.nxs')]
    savedir = getSaveDir()
    for filename in Files:
        absfile = os.path.join(savedir, filename)
        if os.path.exists(absfile):
            os.remove(absfile)
    for direc in ['d_spacing', 'gsas', 'fullprof', 'nexus']:
        direc = os.path.join(savedir, direc)
        if os.listdir(direc) == []:
            os.rmdir(direc)

    return True


def _assert_reduction_configuration(properties_in):
    r"""
    @param properties_in : dictionary with property names and values
    """
    with tempfile.TemporaryDirectory() as save_dir:
        SNAPReduce(EnableConfigurator=True, ConfigSaveDir=save_dir, **properties_in)
        config_files = [str(filename) for filename in Path(save_dir).glob('*.json')]
        properties_out = json.load(open(config_files[0], 'r'))
        for p in properties_out:  # the reciprocal for loop is not true
            assert p in properties_in, f'Property {p} not found in the set of properties passed on to SNAPReduce'


class SNAPReduxShort(systemtesting.MantidSystemTest):

    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        return ['SNAP_34172_event.nxs']

    def runTest(self):
        # run the actual code
        kwargs = dict(RunNumbers='34172', Masking='Horizontal', Binning='0.7,-0.004,5.5',
                      Normalization='Extracted from Data', PeakClippingWindowSize=7,
                      SmoothingRange=5, GroupDetectorsBy='2_4 Grouping', SaveData=True, OutputDirectory=getSaveDir())
        _assert_reduction_configuration(kwargs)
        SNAPReduce(**kwargs)

    def validate(self):
        self.tolerance = 1.0e-2
        self.disableChecking.append('Instrument')  # doesn't validate correctly
        # default validation of workspace to processed nexus is right
        return 'SNAP_34172_2_4_Grouping_nor', 'SNAP_34172_2_4_Grouping_nor.nxs'


class SNAPReduxShortDetcal(systemtesting.MantidSystemTest):
    def skipTests(self):
        return _skip_test()

    def cleanup(self):
        do_cleanup()
        return True

    def requiredFiles(self):
        return ['SNAP_34172_event.nxs']

    def runTest(self):
        # run the actual code

        kwargs = dict(RunNumbers='34172', Masking='Horizontal', Binning='0.7,-0.004,5.5',
                      Calibration='DetCal File', DetCalFilename='SNAP_34172.DetCal',
                      Normalization='Extracted from Data', PeakClippingWindowSize=7,
                      SmoothingRange=5, GroupDetectorsBy='2_4 Grouping',
                      SaveData=True, OutputDirectory=getSaveDir())
        _assert_reduction_configuration(kwargs)
        SNAPReduce(**kwargs)

    def validate(self):
        self.tolerance = 1.0e-2
        self.disableChecking.append('Instrument')  # doesn't validate correctly
        # default validation of workspace to processed nexus is right
        return 'SNAP_34172_2_4_Grouping_nor', 'SNAP_34172_2_4_Grouping_nor.nxs'


class SNAPReduxSimple(systemtesting.MantidSystemTest):
    # this test is very similar to AlignAndFocusPowderFromFilesTest.ChunkingCompare
    def runTest(self):
        # 11MB file, process in 4 chunks
        kwargs = {'RunNumbers': '45874', 'GroupDetectorsBy': 'Banks',  'Binning': [.5, -.004, 7], 'MaxChunkSize': .01}

        # create grouping for two output spectra
        CreateGroupingWorkspace(InstrumentFilename='SNAP_Definition.xml',
                                GroupDetectorsBy='Group',
                                OutputWorkspace='SNAP_grouping')
        _assert_reduction_configuration(kwargs)
        SNAPReduce(**kwargs)
        RenameWorkspace(InputWorkspace='SNAP_45874_Banks_red', OutputWorkspace='with_chunks')

        # process without chunks
        kwargs['MaxChunkSize'] = 0
        SNAPReduce(**kwargs)
        RenameWorkspace(InputWorkspace='SNAP_45874_Banks_red', OutputWorkspace='no_chunks')

    def validateMethod(self):
        #self.tolerance = 1.0e-2
        return "ValidateWorkspaceToWorkspace"

    def validate(self):
        return 'with_chunks', 'no_chunks'
