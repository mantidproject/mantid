import os
import stresstesting
from mantid.simpleapi import *
from mantid import config


def create_file_name(base_name):
    temp_save_dir = config['defaultsave.directory']
    if temp_save_dir == '':
        temp_save_dir = os.getcwd()
    filename = os.path.join(temp_save_dir, base_name + '.nxs')
    return filename


class SaveLoadNexusProcessedTest(stresstesting.MantidStressTest):

    filename = create_file_name('tmp_saveload_nexusprocessed')

    def __init__(self):
        super(SaveLoadNexusProcessedTest, self).__init__()

    def runTest(self):

        CreateSampleWorkspace(OutputWorkspace='ws', Function='Flat background', BankPixelWidth=1, XMax=15,
                              BinWidth=0.75, NumBanks=4)
        MaskBins(InputWorkspace='ws', OutputWorkspace='ws', XMin=0, XMax=2, SpectraList='0')
        MaskBins(InputWorkspace='ws', OutputWorkspace='ws', XMin=0, XMax=1.5, SpectraList='1')
        SaveNexusProcessed(InputWorkspace='ws',Filename=self.filename)
        LoadNexusProcessed(Filename=self.filename,OutputWorkspace='loaded')

        self.tearDown()

    def tearDown(self):
        if os.path.exists(self.filename):
            os.remove(self.filename)

    def validate(self):
        result = CompareWorkspaces('ws','loaded',CheckInstrument=False)
        return result[0]
