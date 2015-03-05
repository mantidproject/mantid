import stresstesting
from mantid.simpleapi import *

class PEARLPowderDiffraction(stresstesting.MantidStressTest):
    
  sample = "PEARL00073987.raw"
  calfile = "pearl_offset_11_4.cal"
  groupfile = "pearl_group_11_2_TT88.cal"
  reffile = "PEARLPowderDiffraction.nxs"

  def requiredFiles(self):
    return [self.sample, self.calfile, self.groupfile, self.reffile]

  def runTest(self):
    LoadRaw(Filename=self.sample, OutputWorkspace='work',LoadLogFiles='0')
    ConvertUnits(InputWorkspace='work',OutputWorkspace='work',Target='Wavelength')

    LoadRaw(Filename=self.sample, OutputWorkspace='monitor73987',LoadLogFiles='0',SpectrumMax='1')
    ConvertUnits(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',Target='Wavelength')
    CropWorkspace(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',
                  XMin=0.03,XMax=6.0)

    MaskBins(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',XMin=3.45,XMax=3.7)
    MaskBins(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',XMin=2.96,XMax=3.2)
    MaskBins(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',XMin=2.1,XMax=2.26)
    MaskBins(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',XMin=1.73,XMax=1.98)

    SplineBackground(InputWorkspace='monitor73987',OutputWorkspace='monitor73987',NCoeff=20)
    NormaliseToMonitor(InputWorkspace='work',OutputWorkspace='work',MonitorWorkspace='monitor73987',
                       IntegrationRangeMin=0.6,IntegrationRangeMax=5.0)
    ConvertUnits(InputWorkspace='work',OutputWorkspace='work',Target='TOF')
    
    rb_params = [1500,-0.0006,19900]
    Rebin(InputWorkspace='work',OutputWorkspace='work',Params=rb_params)
    AlignDetectors(InputWorkspace='work',OutputWorkspace='work', CalibrationFile=self.calfile)
    DiffractionFocussing(InputWorkspace='work',OutputWorkspace='focus',
                         GroupingFileName=self.groupfile)
    
    ConvertUnits(InputWorkspace='focus',OutputWorkspace='focus',Target='TOF')
    Rebin(InputWorkspace='focus',OutputWorkspace='focus',Params=rb_params)
    CropWorkspace(InputWorkspace='focus',OutputWorkspace='focus',XMin=0.1)

  def validate(self):
    return 'focus','PEARLPowderDiffraction.nxs'
