"""*WIKI* 

Finds spectrum numbers corresponding to reflected and transmission lines in a line detector Reflectometry dataset.

Expects two or one, reflectometry peaks, will fail if there are more or less than this number of peaks. The first peak is taken to be from the reflected line, the second is taken to be from the transmission line. This  algorithm outputs a  Table workspace containing the spectrum number of interest.
*WIKI*"""

from mantid.api import *
from mantid.kernel import *
import numpy as np

class PeakFindingException(Exception):
	pass

class FindReflectometryLines(PythonAlgorithm):
	
	def __is_peak(self, before, current, after):
		if before < current and current > after:
			return True
		return False
		
	def __find_peak_spectrum_numbers(self, y_data, ws):
		peak_index_list = []
		for index, current in enumerate(y_data.flat):
			if (index > 0) and (index < (y_data.size - 1)):
				before = y_data[index-1]
				after = y_data[index+1]
				if self.__is_peak(before, current, after):
					spec_number = ws.getSpectrum(index).getSpectrumNo()
					peak_index_list.append(spec_number)
		return peak_index_list
		
	def __remove_background(self, y_data):
		y_average = np.sum(y_data) / y_data.size
		y_max = np.max(y_data)
		threshold =  (y_max - y_average)/10 + y_average
		y_data[y_data < threshold] = 0
		return y_data
	
	def category(self):
		return "PythonAlgorithms"

	def name(self):
		return "FindReflectometryLines"
	
	def PyInit(self):
		
		workspace_validator = CompositeValidator()
		workspace_validator.add(WorkspaceUnitValidator("Wavelength"))
		workspace_validator.add(SpectraAxisValidator())
		
		self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input, workspace_validator), "Input Reflectometry Workspace")
		self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output Spectrum Numbers")
		self.declareProperty(name="StartWavelength", defaultValue=0.0, validator=FloatBoundedValidator(lower=0.0),  doc="Start wavelength to use for x-axis cropping")	
		self.declareProperty(name="KeepIntermediateWorkspaces", defaultValue=False, doc="Keeps cropped and integrated workspaces in memory after usage.")
	
	def PyExec(self):
		
		in_ws = self.getPropertyValue("InputWorkspace")
		min_wavelength = self.getPropertyValue("StartWavelength")
		keep_workspaces = self.getPropertyValue("KeepIntermediateWorkspaces")
		
		cropped_ws = CropWorkspace(InputWorkspace=in_ws,XMin=float(min_wavelength))
		summed_ws = Integration(InputWorkspace=cropped_ws)
		
		n_histograms = summed_ws.getNumberHistograms()
		y_data = np.empty([n_histograms])
		for i in range(0, n_histograms):
			intensity = summed_ws.readY(i)[0]
			y_data[i] = intensity
			
		y_data = self.__remove_background(y_data)
		peak_index_list = self.__find_peak_spectrum_numbers(y_data, summed_ws)
		n_peaks_found = len(peak_index_list) 
		
		output_ws = WorkspaceFactory.createTable("TableWorkspace")
		output_ws.addColumn("int", "Reflected Spectrum Number")
		
		if n_peaks_found > 2:
			raise PeakFindingException("Found more than two peaks.")
		elif n_peaks_found == 0:
			raise PeakFindingException("No peaks found")
		elif n_peaks_found == 1:
			output_ws.addRow(peak_index_list)
		elif n_peaks_found == 2:
			output_ws.addColumn("int", "Transmission Spectrum Number")
			output_ws.addRow(peak_index_list)
		
		if int(keep_workspaces) == 0:
			DeleteWorkspace(Workspace=cropped_ws)
			DeleteWorkspace(Workspace=summed_ws)
		
	        self.setProperty("OutputWorkspace", output_ws)
		
registerAlgorithm(FindReflectometryLines())