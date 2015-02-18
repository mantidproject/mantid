from mantid.api import *
from mantid.kernel import *
import numpy as np

class PeakFindingException(Exception):
    pass

class FindReflectometryLines(PythonAlgorithm):

    # Determine if a given signal  identifies a peak or not.
    def __is_peak(self, before, current, after):
    	# A peak is defined to be any signal that is preceeded by a lower signal value and followed by a lower signal value.
        if before < current and current > after:
            return True
        return False

    # Find a list of spectra numbers corresponding to peaks in the data.
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

    # Zero-out any data considered to be background.
    def __remove_background(self, y_data):
        y_average = np.sum(y_data) / y_data.size
        y_max = np.max(y_data)
    	#The thresholding criteria is hard-coded to be above the average as follows.
        threshold =  (y_max - y_average)/10 + y_average
        y_data[y_data < threshold] = 0
        return y_data

    def category(self):
        return "PythonAlgorithms;Reflectometry"

    def name(self):
        return "FindReflectometryLines"

    def summary(self):
        return "Finds spectrum numbers corresponding to reflected and transmission lines in a line detector Reflectometry dataset."

    def PyInit(self):

        workspace_validator = CompositeValidator()
        workspace_validator.add(WorkspaceUnitValidator("Wavelength"))
        workspace_validator.add(SpectraAxisValidator())

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input, workspace_validator), "Input Reflectometry Workspace")
        self.declareProperty(ITableWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output Spectrum Numbers")
        self.declareProperty(name="StartWavelength", defaultValue=0.0, validator=FloatBoundedValidator(lower=0.0),  doc="Start wavelength to use for x-axis cropping")
        self.declareProperty(name="KeepIntermediateWorkspaces", defaultValue=False, doc="Keeps cropped and integrated workspaces in memory after usage.")

    def PyExec(self):
        from mantid.simpleapi import CropWorkspace, Integration, DeleteWorkspace

        in_ws = self.getPropertyValue("InputWorkspace")
        min_wavelength = self.getPropertyValue("StartWavelength")
        keep_workspaces = self.getPropertyValue("KeepIntermediateWorkspaces")

    	# Crop off lower wavelengths where the signal is also lower.
        cropped_ws = CropWorkspace(InputWorkspace=in_ws,XMin=float(min_wavelength))
    	# Integrate over the higher wavelengths after cropping.
        summed_ws = Integration(InputWorkspace=cropped_ws)
    	# Loop through each histogram, and fetch out each intensity value from the single bin to generate a list of all values.
        n_histograms = summed_ws.getNumberHistograms()
        y_data = np.empty([n_histograms])
        for i in range(0, n_histograms):
            intensity = summed_ws.readY(i)[0]
            y_data[i] = intensity
    	#Remove the background
        y_data = self.__remove_background(y_data)
    	#Find the peaks
        peak_index_list = self.__find_peak_spectrum_numbers(y_data, summed_ws)
        #Reverse the order so that it goes from high spec number to low spec number
        peak_index_list.reverse()
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

AlgorithmFactory.subscribe(FindReflectometryLines())
