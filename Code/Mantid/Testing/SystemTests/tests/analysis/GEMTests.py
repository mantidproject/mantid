import stresstesting
import os
from mantid.simpleapi import *

class GEMTest(stresstesting.MantidStressTest):

	def __init__(self):
		stresstesting.MantidStressTest.__init__(self)
		self.gss_file = ''
		self.ref_gss_file = 'GEM58654.gss'
		self.xye_tof_files = []
		self.ref_xye_tof_files = ['GEM58654_b1_TOF.dat','GEM58654_b2_TOF.dat','GEM58654_b3_TOF.dat','GEM58654_b4_TOF.dat','GEM58654_b5_TOF.dat','GEM58654_b6_TOF.dat']
		self.xye_d_files = []
		self.ref_xye_d_files = ['GEM58654_b1_D.dat','GEM58654_b2_D.dat','GEM58654_b3_D.dat','GEM58654_b4_D.dat','GEM58654_b5_D.dat','GEM58654_b6_D.dat']
		self.file_index = 0
		self.new_cal_file = ''

	def runTest(self):
		# do something
		LoadRaw(Filename=r'GEM59378.raw',OutputWorkspace='Vanadium',LoadLogFiles='0')
		CreateSingleValuedWorkspace(OutputWorkspace='totuamps',DataValue='450.02215576200001')
		Divide(LHSWorkspace='Vanadium',RHSWorkspace='totuamps',OutputWorkspace='Vanadium')
		SolidAngle(InputWorkspace='Vanadium',OutputWorkspace='Corr')
		CreateSingleValuedWorkspace(OutputWorkspace='Sc',DataValue='100')
		Multiply(LHSWorkspace='Corr',RHSWorkspace='Sc',OutputWorkspace='Corr')
		Divide(LHSWorkspace='Vanadium',RHSWorkspace='Corr',OutputWorkspace='Vanadium')
		ConvertUnits(InputWorkspace='Vanadium',OutputWorkspace='Vanadium',Target='Wavelength')
		Integration(InputWorkspace='Vanadium',OutputWorkspace='Vanadium',RangeLower='1.3999999999999999',RangeUpper='3')
		Multiply(LHSWorkspace='Corr',RHSWorkspace='Vanadium',OutputWorkspace='Corr')
                DeleteWorkspace('Vanadium')
		CreateSingleValuedWorkspace(OutputWorkspace='Sc',DataValue='100000')
		Divide(LHSWorkspace='Corr',RHSWorkspace='Sc',OutputWorkspace='Corr')
                
                self.new_cal_file = os.path.join(config['defaultsave.directory'],'offsets_2011_cycle111b_new.cal')
		MaskDetectorsIf(InputWorkspace='Corr',Mode='DeselectIf',InputCalFile=r'offsets_2011_cycle111b.cal',OutputCalFile=self.new_cal_file)
		# load precompiled vanadium files
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-0.nxs',OutputWorkspace='Vanadium-1')
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-1.nxs',OutputWorkspace='Vanadium-2')
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-2.nxs',OutputWorkspace='Vanadium-3')
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-3.nxs',OutputWorkspace='Vanadium-4')
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-4.nxs',OutputWorkspace='Vanadium-5')
		LoadNexusProcessed(Filename=r'van_gem59378_benchmark-5.nxs',OutputWorkspace='Vanadium-6')
		# load data
		LoadRaw(Filename=r'GEM58654.raw',OutputWorkspace='sample',LoadLogFiles='0')
		LoadRaw(Filename=r'GEM58654.raw',OutputWorkspace='sampleadd',LoadLogFiles='0')
		Plus(LHSWorkspace='sampleadd',RHSWorkspace='sample',OutputWorkspace='sample')
                DeleteWorkspace('sampleadd')
		CreateSingleValuedWorkspace(OutputWorkspace='totuamps',DataValue='600.05676269499997')
		Divide(LHSWorkspace='sample',RHSWorkspace='totuamps',OutputWorkspace='sample')

		LoadRaw(Filename=r'GEM59381.raw',OutputWorkspace='Sempty',LoadLogFiles='0')
		CreateSingleValuedWorkspace(OutputWorkspace='totuamps',DataValue='400.04138183600003')
		Divide(LHSWorkspace='Sempty',RHSWorkspace='totuamps',OutputWorkspace='Sempty')
		Minus(LHSWorkspace='sample',RHSWorkspace='Sempty',OutputWorkspace='sample')
                DeleteWorkspace('Sempty')
		AlignDetectors(InputWorkspace='sample',OutputWorkspace='sample',CalibrationFile=r'offsets_2011_cycle111b.cal')
		Divide(LHSWorkspace='sample',RHSWorkspace='Corr',OutputWorkspace='sample')
                DeleteWorkspace('Corr')
		CreateSingleValuedWorkspace(OutputWorkspace='scale',DataValue='1')
		Multiply(LHSWorkspace='sample',RHSWorkspace='scale',OutputWorkspace='sample')
		ConvertUnits(InputWorkspace='sample',OutputWorkspace='sample',Target='Wavelength')
		CylinderAbsorption(InputWorkspace='sample',OutputWorkspace='SampleTrans',AttenuationXSection='0.5',ScatteringXSection='1',SampleNumberDensity='1',NumberOfWavelengthPoints='100',CylinderSampleHeight='4',CylinderSampleRadius='0.40000000000000002',NumberOfSlices='10',NumberOfAnnuli='10')
		Divide(LHSWorkspace='sample',RHSWorkspace='SampleTrans',OutputWorkspace='sample')
		ConvertUnits(InputWorkspace='sample',OutputWorkspace='sample',Target='dSpacing')
		DiffractionFocussing(InputWorkspace='sample',OutputWorkspace='sample',GroupingFileName=self.new_cal_file)
		
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-1',EndWorkspaceIndex='0')
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-2',StartWorkspaceIndex='1',EndWorkspaceIndex='1')
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-3',StartWorkspaceIndex='2',EndWorkspaceIndex='2')
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-4',StartWorkspaceIndex='3',EndWorkspaceIndex='3')
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-5',StartWorkspaceIndex='4',EndWorkspaceIndex='4')
		CropWorkspace(InputWorkspace='sample',OutputWorkspace='sample-6',StartWorkspaceIndex='5',EndWorkspaceIndex='5')
                DeleteWorkspace('sample')
		Divide(LHSWorkspace='sample-1',RHSWorkspace='Vanadium-1',OutputWorkspace='ResultD-1')
		Divide(LHSWorkspace='sample-2',RHSWorkspace='Vanadium-2',OutputWorkspace='ResultD-2')
		Divide(LHSWorkspace='sample-3',RHSWorkspace='Vanadium-3',OutputWorkspace='ResultD-3')
		Divide(LHSWorkspace='sample-4',RHSWorkspace='Vanadium-4',OutputWorkspace='ResultD-4')
		Divide(LHSWorkspace='sample-5',RHSWorkspace='Vanadium-5',OutputWorkspace='ResultD-5')
		Divide(LHSWorkspace='sample-6',RHSWorkspace='Vanadium-6',OutputWorkspace='ResultD-6')
		Rebin(InputWorkspace='ResultD-1',OutputWorkspace='ResultD-1',Params='0.559211,-0.004,37.6844')
		Rebin(InputWorkspace='ResultD-2',OutputWorkspace='ResultD-2',Params='0.348675,-0.002,14.5631')
		Rebin(InputWorkspace='ResultD-3',OutputWorkspace='ResultD-3',Params='0.169661,-0.0011546,8.06311')
		Rebin(InputWorkspace='ResultD-4',OutputWorkspace='ResultD-4',Params='0.108284,-0.00111682,4.25328')
		Rebin(InputWorkspace='ResultD-5',OutputWorkspace='ResultD-5',Params='0.0818697,-0.00109142,2.82906')
		Rebin(InputWorkspace='ResultD-6',OutputWorkspace='ResultD-6',Params='0.0661098,-0.00105175,1.87008')
		ConvertUnits(InputWorkspace='ResultD-1',OutputWorkspace='ResultTOF-1',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-1',OutputWorkspace='ResultD-1',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-1',OutputWorkspace='ResultTOF-1',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ConvertUnits(InputWorkspace='ResultD-2',OutputWorkspace='ResultTOF-2',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-2',OutputWorkspace='ResultD-2',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-2',OutputWorkspace='ResultTOF-2',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ConvertUnits(InputWorkspace='ResultD-3',OutputWorkspace='ResultTOF-3',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-3',OutputWorkspace='ResultD-3',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-3',OutputWorkspace='ResultTOF-3',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ConvertUnits(InputWorkspace='ResultD-4',OutputWorkspace='ResultTOF-4',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-4',OutputWorkspace='ResultD-4',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-4',OutputWorkspace='ResultTOF-4',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ConvertUnits(InputWorkspace='ResultD-5',OutputWorkspace='ResultTOF-5',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-5',OutputWorkspace='ResultD-5',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-5',OutputWorkspace='ResultTOF-5',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ConvertUnits(InputWorkspace='ResultD-6',OutputWorkspace='ResultTOF-6',Target='TOF')
		ReplaceSpecialValues(InputWorkspace='ResultD-6',OutputWorkspace='ResultD-6',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		ReplaceSpecialValues(InputWorkspace='ResultTOF-6',OutputWorkspace='ResultTOF-6',NaNValue='0',InfinityValue='0',BigNumberThreshold='99999999.999999985')
		
		# group and save
		GroupWorkspaces(InputWorkspaces='ResultTOF-1,ResultTOF-2,ResultTOF-3,ResultTOF-4,ResultTOF-5,ResultTOF-6',OutputWorkspace='ResultTOFgrp')

                self.gss_file = os.path.join(config['defaultsave.directory'],'GEM58654_new.gss')
                append=False
                for i in range(1,7):
                    if i > 1:
                        append=True
                    SaveGSS(InputWorkspace='ResultTOF-%d' % i,Filename=self.gss_file,SplitFiles=False,Append=append,Bank=i)
                    
                    filename= os.path.join(config['defaultsave.directory'],r'GEM58654_b%d_TOF.dat' % i)
                    SaveFocusedXYE(InputWorkspace='ResultTOF-%d' % i,Filename=filename,SplitFiles=False,IncludeHeader='0')
                    self.xye_tof_files.append(filename)
                    
                    filename= os.path.join(config['defaultsave.directory'],r'GEM58654_b%d_D.dat' % i)
                    SaveFocusedXYE(InputWorkspace='ResultD-%d' % i,Filename=filename,SplitFiles=False,IncludeHeader='0')
                    self.xye_d_files.append(filename)

	def cleanup(self):
            '''Remove temporary files'''
	    if os.path.exists(self.gss_file):
                os.remove(self.gss_file)
            if os.path.exists(self.new_cal_file):
                os.remove(self.new_cal_file)
	    for file in self.xye_tof_files:
                if os.path.exists(file):
		    os.remove(file)
            for file in self.xye_d_files:
                if os.path.exists(file):
                    os.remove(file)
			
	def doValidation(self):
		'''Override doValidation to vaildate two things at the same time'''
		self.disableChecking.append('Instrument')
		# reset validate() method to call validateNexus() instead
		self.validate = self.validateNexus
		res = self.validateWorkspaceToNeXus()
		if not res:
			return False
		# reset validate() method to call validateGSS()
		self.validate = self.validateGSS
		res = self.validateASCII()
		if not res:
			return False
		# reset validate() method to call validateTOFXYE()
		self.validate = self.validateTOFXYE
		self.file_index = 0
		# file_index is incremented after each call to validateASCII() 
		res = self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII()
		if not res:
			return False
		# reset validate() method to call validateTOFXYE()
		self.validate = self.validateDXYE
		self.file_index = 0
		# file_index is incremented after each call to validateASCII() 
		res = self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII() and self.validateASCII()
		return res

	def validateNexus(self):
		'''Compare the result of reduction with the reference nexus file'''
		return 'ResultTOFgrp','GEM58654.nxs'

	def validateGSS(self):
		'''Validate the created gss file'''
		from mantid.api import FileFinder
		return self.gss_file, FileFinder.getFullPath(self.ref_gss_file)

	def validateTOFXYE(self):
		'''Validate the created gss file'''
		from mantid.api import FileFinder
		i = self.file_index
		self.file_index += 1
		return self.xye_tof_files[i], FileFinder.getFullPath(self.ref_xye_tof_files[i])

	def validateDXYE(self):
		'''Validate the created gss file'''
		from mantid.api import FileFinder
		i = self.file_index
		self.file_index += 1
		return self.xye_d_files[i], FileFinder.getFullPath(self.ref_xye_d_files[i])

