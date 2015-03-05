import stresstesting
from mantid.simpleapi import *
from mantid.api import FileFinder
from mantid import config
import ISISCommandInterface as ici
import SANSBatchMode as batch
import SANSadd2 as sansadd

import os

class SANSAddBatch(stresstesting.MantidStressTest):
  output_file = '99630sannotrans'
  csv_file = 'input.csv'
  result = ''

  def cleanup(self):
    print "Cleanup"
    absfile = FileFinder.getFullPath("input.csv")
    if os.path.exists(absfile):
      os.remove(absfile)
    return True

  def runTest(self):
    #here we are testing the LOQ setup
    ici.LOQ()
    #rear detector
    ici.Detector("main-detector-bank")
    #test batch mode, although only the analysis from the last line is checked
    # Find the file , this should really be in the BatchReduce reduction step

    f = open(self.csv_file,'w')
    print >> f, "sample_sans,99630-add,output_as, %s"%self.output_file
    f.close()
    runnum = '99630'
    sansadd.add_runs((runnum, runnum),'LOQ','.RAW')

    ici.Set1D()
    ici.MaskFile('MASK.094AA')    
    batch.BatchReduce(self.csv_file, 'nxs', plotresults=False, saveAlgs={'SaveNexus':'nxs'})
    
    print ' reduction without'

    ici._refresh_singleton()
    
    ici.LOQ()
    ici.Detector("main-detector-bank")
    ici.Set1D()
    ici.MaskFile('MASK.094AA')
    LOQ99630 = Load(runnum)
    LOQ99630 += LOQ99630 
    ici.AssignSample(LOQ99630, reload=False)
    self.result = ici.WavRangeReduction()
    
  def validate(self):
    # Need to disable checking of the Spectra-Detector map because it isn't
    # fully saved out to the nexus file (it's limited to the spectra that
    # are actually present in the saved workspace).
    self.disableChecking.append('SpectraMap')
    self.disableChecking.append('Axes')
    self.disableChecking.append('Instrument')
    self.tolerance = 1.0e-10 #almost ZERO!
    print 'validating', self.result, self.output_file
    return self.result,self.output_file+'.nxs'



  def __del__(self):
      # remove all created files.
      defaultsave = config['defaultsave.directory']
      for file_name in ['LOQ99630-add.nxs', self.output_file+'.nxs', self.csv_file ]:
          try:
              os.remove(os.path.join(defaultsave,file_name))
          except:
              pass

    
