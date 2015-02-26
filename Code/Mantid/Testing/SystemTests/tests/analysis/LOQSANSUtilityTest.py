import stresstesting
from mantid.simpleapi import *
from mantid import config
import SANSUtility as su
import SANSadd2 as add

import os

def unixLikePathFromWorkspace(ws):
    return su.getFilePathFromWorkspace(ws).replace('\\','/')


class SANSUtilityTest(stresstesting.MantidStressTest):

    def runTest(self):
        # created after issue reported in #8156
        ws = Load('LOQ54432')
        self.assertTrue('Data/SystemTest/LOQ/LOQ54432.raw' in unixLikePathFromWorkspace(ws))
        ws = Load('LOQ99618.RAW')
        self.assertTrue('Data/SystemTest/LOQ/LOQ99618.RAW' in unixLikePathFromWorkspace(ws))
        add.add_runs(('LOQ54432','LOQ54432'),'LOQ','.raw')
        ws = Load('LOQ54432-add')
        file_path =  unixLikePathFromWorkspace(ws)
        logger.information("File Path from -add: "+str(file_path))
        file_path = file_path.replace('-ADD','-add') # MAC seems to report that the file is LOQ54432-ADD.nxs
        self.assertTrue('LOQ54432-add' in file_path)
        os.remove(file_path)
        
