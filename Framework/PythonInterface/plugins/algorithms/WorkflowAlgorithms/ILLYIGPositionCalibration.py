# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import *
from mantid.kernel import Direction, StringListValidator
from mantid.api import FileAction, FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    NumericAxis, Progress, PropertyMode, PythonAlgorithm, WorkspaceProperty
from datetime import date
import math
import numpy as np
import os
from xml.dom import minidom
import xml.etree.ElementTree as ET


class ILLYIGPositionCalibration(PythonAlgorithm):

    # helper conversions
    _RAD_2_DEG = 180.0 / np.pi
    _DEG_2_RAD = 1.0 / _RAD_2_DEG
    # number of detectors
    _D7NumberBanks = 3
    _D7NumberPixels = 132
    _D7NumberPixelsBank = 44
    # an approximate universal peak width:
    _PeakWidth = 2.0 # in degrees

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs D7 position calibration using YIG scan and returns D7 IPF readable by the NeXus data loader.'

    def seeAlso(self):
        return ['LoadILLPolarizedDiffraction']

    def name(self):
        return 'ILLYIGPositionCalibration'

    def validateInputs(self):
        issues = dict()
        if (self.getProperty('Filenames').isDefault
                and self.getProperty('ScanWorkspace').isDefault):
            issues['Filenames'] = 'Either a list of filenames containing YIG scan \
                or the workspace with the loaded scan is required for calibration.'
            issues['ScanWorkspace'] = issues['Filenames']

        return issues

    def PyInit(self):
        self.declareProperty(MultipleFileProperty("Filenames",
                                                  action=FileAction.OptionalLoad),
                             doc="The file names (including full or relative path) with a single YIG scan.")

        self.declareProperty(MatrixWorkspaceProperty('ScanWorkspace', '',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='The name of the workspace containing the entire YIG scan.')

        self.declareProperty(FileProperty('YIGPeaksFile', '',
                                          action=FileAction.Load,
                                          extensions=['.xml']),
                             doc='The file name file with all YIG peaks in d-spacing.')

        self.declareProperty(name="ApproximateWavelength",
                             defaultValue="3.1",
                             validator=StringListValidator(["3.1", "4.8", "5.7"]),
                             direction=Direction.Input,
                             doc="The initial guess for the neutrons' wavelength")

        self.declareProperty(name='CalibrationFilename',
                             defaultValue='',
                             direction=Direction.Input,
                             doc="The output YIG calibration Instrument Parameter File name.")

        self.declareProperty(WorkspaceProperty('DetectorFitOutput', '',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='The table workspace name that will be used to store all of the calibration parameters.')

    def PyExec(self):
        progress = Progress(self, start=0.0, end=1.0, nreports=5)
        # load the chosen YIG scan
        if self.getProperty('ScanWorkspace').isDefault:
            intensityWS = self._get_scan_data()
        else:
            intensityWS = self.getProperty('ScanWorkspace').value
        progress.report()
        # load the YIG peaks from an IPF
        yig_d = self._load_yig_peaks(intensityWS)
        # check how many and which peaks can be fitted in each row
        yig_peaks_positions = self._get_yig_peaks_positions(intensityWS, yig_d)
        progress.report()
        # fit gaussian to peaks for each pixel, returns peaks as a function of their expected position
        peaks_positions = self._fit_bragg_peaks(intensityWS, yig_peaks_positions)
        progress.report()
        # fit the wavelegnth, bank gradients and individual
        detector_parameters = self._fit_detector_positions(peaks_positions)
        progress.report()
        # fit the even and odd detectors to get position distributions in banks
        wavelength, pixel_offsets = self._calibrate_detectors(detector_parameters)
        progress.report()
        # print the Instrument Parameter File that can be used in the ILLPolarizedDiffraction loader
        self._print_parameter_file(wavelength, pixel_offsets)
        progress.report()

        if self.getProperty('DetectorFitOutput').isDefault:
            pass #will clean up the remaining workspaces
        else:
            self.setProperty('DetectorFitOutput', mtd['det_out_Parameters'])

    def _get_scan_data(self):
        """ Loads YIG scan data, removes monitors, and prepares
        a workspace for Bragg peak fitting"""

        # workspace indices for monitors
        MONITOR_INDICES = "{0}, {1}".format(self._D7NumberPixels, self._D7NumberPixels+1)

        outGroup = Load(self.getPropertyValue("Filenames"),
                        OutputWorkspace="outGroup",
                        PositionCalibration='None')
        # load the group into a single table workspace
        nfiles = outGroup.getNumberOfEntries()
        # new vertical axis
        x_axis = NumericAxis.create(nfiles)
        # Fill the intensity and position tables with all the data from scans
        for scan in range(nfiles):
            workspace = outGroup.getItem(scan)
#             # normalize to monitor1 as monitor2 is sometimes empty:
            if workspace.readY(workspace.getNumberHistograms()-2)[0] != 0:
                workspace /= workspace.readY(workspace.getNumberHistograms()-2)[0]
            # remove Monitors:
            workspace = RemoveSpectra(workspace, MONITOR_INDICES)
            # prepare proper label for the axes
            x_axis_label = workspace.run().getProperty('2theta.requested').value
            x_axis.setValue(scan, x_axis_label)
            # convert the x-axis to signedTwoTheta
            workspace = ConvertAxisByFormula(workspace,
                                             Axis='X',
                                             Formula='-180.0 * signedtwotheta / pi')
            # mask detectors too close to the beam axis or too far away:
            workspace = MaskBinsIf(workspace, Criterion='x<15.0 && x>-35.0')
            # append the new row to a new MatrixWorkspace
            workspace = ConvertToPointData(InputWorkspace=workspace)
            try:
                intensityWS
            except NameError:
                intensityWS = workspace.clone()
            else:
                list = [intensityWS, workspace]
                intensityWS = ConjoinXRuns(list)

        #replace axis and corrects labels
        x_axis.setUnit("Label").setLabel('TwoTheta', 'degrees')
        intensityWS.replaceAxis(0, x_axis)
        y_axis = NumericAxis.create(self._D7NumberPixels)
        for pixel_no in range(self._D7NumberPixels):
            y_axis.setValue(pixel_no, pixel_no+1)
        intensityWS.replaceAxis(1, y_axis)
        return intensityWS

    def _load_yig_peaks(self, ws):
        """Loads YIG peaks provided as an XML Instrument Parameter File"""
        parameterFilename = self.getPropertyValue('YIGPeaksFile')
        LoadParameterFile(Workspace=ws, Filename=parameterFilename)
        yig_d_list = []
        instrument = ws.getInstrument().getComponentByName('detector')
        for param_name in instrument.getParameterNames(True):
            yig_d_list.append(instrument.getNumberParameter(param_name)[0])
        return sorted(yig_d_list)

    def _remove_invisible_yig_peaks(self, yig_list):
        """Removes YIG peaks with 2theta above 180 degrees
        and returns a list with peaks positions in 2theta"""
        wavelength = float(self.getPropertyValue("ApproximateWavelength"))
        yig_list = [self._RAD_2_DEG * math.asin(wavelength / (2.0 * d_spacing))
                    for d_spacing in yig_list if d_spacing > 0.5*wavelength]
        return self._include_other_quadrants(yig_list)

    def _include_other_quadrants(self, yig_list):
        """Adds other quadrants to the peak list: (-90,0) degrees"""
        peak_list = []
        for peak in yig_list:
            peak_list.append(peak)
            peak_list.append(-peak)
        peak_list.sort()
        return peak_list

    def _get_yig_peaks_positions(self, ws, YIG_D):
        """Returns a list o tuples with peak centre positions and peak height
        used for further fitting. Removes all peaks that would require scattering
        angle above 180 degrees and returns the peak positions in degrees"""
        yig2theta = self._remove_invisible_yig_peaks(YIG_D)

        peak_list = []
        for scan in range(ws.getNumberHistograms()):
            detector_2theta = ws.readX(scan)
            intensity = ws.readY(scan)
            min2Theta = detector_2theta[0]
            max2Theta = detector_2theta[-1]
            single_spectrum_peaks = []
            for peak in yig2theta:
                if ( ((peak - self._PeakWidth) > min2Theta ) and ( (peak + self._PeakWidth) < max2Theta) ):
                    peak_intensity = 0
                    for pixel_no in range(len(detector_2theta)):
                        twoTheta = detector_2theta[pixel_no]
                        if (abs(twoTheta-peak) < 1): # within 1 degree from the expected peak position
                            peak_intensity = intensity[pixel_no]
                            if (peak_intensity != 0): # exclude masked peaks
                                single_spectrum_peaks.append((peak_intensity, twoTheta))
                            break
            peak_list.append(single_spectrum_peaks)
        return peak_list

    def _fit_bragg_peaks(self, ws, yig_peaks):
        """ Fits peaks defined in the yig_peaks argument
        returns a workspace with fitted peak positions
        on the Y axis and the expected positions on the X axis"""
        max_n_peaks = len(max(yig_peaks, key=len))
        for scan in range(ws.getNumberHistograms()):
            if len(yig_peaks[scan]) == 0:
                continue
            single_spectrum_peaks = yig_peaks[scan]
            # fit all functions simultaneously:
            function="name=FlatBackground, A0=0;\n"
            constraints = "f0.A0 >= 0"
            function_no = 1
            for peak_intensity, peak_centre in single_spectrum_peaks:
                function += "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2};\n".format(
                                                                                             peak_centre,
                                                                                             peak_intensity,
                                                                                             0.5*self._PeakWidth)
                constraints += ",f{0}.Height > 0.0".format(function_no)
                constraints += ",f{0}.Sigma < 2.0".format(function_no)
                constraints += ",{0} < f{1}.PeakCentre < {2}".format(float(peak_centre)-self._PeakWidth*2,
                                                                     function_no,
                                                                     float(peak_centre)+self._PeakWidth*2)
                function_no += 1
            fit_output = Fit(Function=function,
                             InputWorkspace=ws,
                             WorkspaceIndex=scan,
                             Constraints=constraints,
                             CreateOutput=True,
                             Output='out')
            param_table = fit_output.OutputParameters
            # create the needed columns in the output workspace
            results_x = np.zeros(max_n_peaks)
            results_y = np.zeros(max_n_peaks)
            results_e = np.zeros(max_n_peaks)
            peak_no = 0
            for row_no in range(param_table.rowCount()):
                row_data = param_table.row(row_no)
                if 'PeakCentre' in row_data['Name']:
                    intensity, peak_position = single_spectrum_peaks[peak_no]
                    if (param_table.row(row_no-1)['Value'] > 0):
                        results_x[peak_no] = peak_position
                        results_y[peak_no] = row_data['Value']
                        results_e[peak_no] = row_data['Error']
                    peak_no += 1
            try:
                fit_results
            except NameError:
                fit_results_exists = False
                fit_results = CreateWorkspace(DataX=results_x,
                                              DataY=results_y,
                                              DataE=results_e,
                                              UnitX='degrees',
                                              NSpec=1)
            else:
                fit_results_exists = True
            if fit_results_exists:
                workspace = CreateWorkspace(DataX=results_x,
                                            DataY=results_y,
                                            DataE=results_e,
                                            UnitX='degrees',
                                            NSpec=1)
                ConjoinWorkspaces(fit_results, workspace,
                                  CheckOverlapping=False,
                                  YAxisLabel='TwoTheta_fit',
                                  YAxisUnit='degrees')
                fit_results = mtd['fit_results']

        y_axis = NumericAxis.create(self._D7NumberPixels)
        for pixel_no in range(self._D7NumberPixels):
            y_axis.setValue(pixel_no, pixel_no+1)
        fit_results.replaceAxis(1, y_axis)

        return fit_results

    def _fit_detector_positions(self, ws):
        """Fits lambda = 2 * d * sin (m * 2theta + offset),
        where lambda, m and offset are parameters,
        to the peak distribution.
        Returns parameter table with fitted wavelength,
        gradient for each bank and offset value for each
        pixel."""

        # need to set up a function for each pixel with proper ties
        function_list = []
        ties_lambda_list = []
        ties_gradient_list = []
        ties_gradient = ""
        offset_constr = self._DEG_2_RAD * 30 # +-30 degrees around the bank position
        gradient_constr = 0.1 # +-10% around the m = 1.0 value
        lambda_constr = 0.05 # +- 5% of lambda variation from the initial assumption
        constraint_list = ['{0}<f0.lambda<{1}'.format(1-lambda_constr, 1+lambda_constr)]

        for pixel_no in range(ws.getNumberHistograms()):
            function = 'name=UserFunction, \
            Formula = {0} * m * ( asin( lambda * sin( {1}*x ) ) + offset), \
            lambda= 1.0, m = 1.0, offset = {2}, $domains=i'.format(self._RAD_2_DEG, self._DEG_2_RAD, 0)
            function_list.append(function)
            constraint_list.append('-{0} < f{1}.offset < {0}'.format(offset_constr,
                                                                     pixel_no))
            constraint_list.append('{0} < f{1}.m < {2}'.format(1-gradient_constr,
                                                               pixel_no,
                                                               1+gradient_constr))
            # add a global tie on lambda:
            ties_lambda_list.append('f{0}.lambda'.format(pixel_no))
            # set ties for three bank gradients:
            ties_gradient_list.append('f{0}.m'.format(pixel_no))
            if pixel_no % self._D7NumberPixelsBank == (self._D7NumberPixelsBank-1):
                ties_gradient += ',' + '='.join(ties_gradient_list)
                ties_gradient_list=[]

        ties_lambda = '='.join(ties_lambda_list)
        ties = ties_lambda + ties_gradient

        constraints = ','.join(constraint_list)
        # create a multi domain function to perform a global fit
        multiFunc='composite=MultiDomainFunction,NumDeriv=true;'

        # define the domains needed by the fitting algorithm
        fit_kwargs = {}
        func_no = 0
        for function in function_list:
            multiFunc += "("+ function + ");\n"
            if func_no == 0:
                fit_kwargs['WorkspaceIndex'] = func_no
                fit_kwargs['InputWorkspace'] = ws
            else:
                fit_kwargs['WorkspaceIndex_%d' % func_no] = func_no
                fit_kwargs['InputWorkspace_%d' % func_no] = ws
            func_no += 1

        multiFunc += 'ties=(' + ties + ')'
        fit_output = Fit(Function=multiFunc,
                         Constraints=constraints,
                         IgnoreInvalidData=True,
                         CreateOutput=True,
                         Output='det_out',
                         **fit_kwargs,)

        param_table = fit_output.OutputParameters

        return param_table

    def _calculate_pixel_positions(self, parameter_table):
        """Calculates pixel positions using provided
        gradients and offsets.
        Returns a list of pixel positions relative to their
        respective bank"""
        intitial_wavelength = self.getProperty('ApproximateWavelength').value
        wavelength = parameter_table.column(1)[1] * float(intitial_wavelength)
        if parameter_table.column(1)[0] == 0:
            raise RuntimeError('Bank2 slope is equal to 0.')
        bank2_slope = 1.0 / float(parameter_table.column(1)[0])
        if parameter_table.column(1)[3*self._D7NumberPixelsBank] == 0:
            raise RuntimeError('Bank3 slope is equal to 0.')
        bank3_slope = 1.0 / float(parameter_table.column(1)[3*self._D7NumberPixelsBank])
        if parameter_table.column(1)[6*self._D7NumberPixelsBank] == 0:
            raise RuntimeError('Bank4 slope is equal to 0.')
        bank4_slope = 1.0 / float(parameter_table.column(1)[6*self._D7NumberPixelsBank])
        bank_slopes = [bank2_slope, bank3_slope, bank4_slope]
        pixel_offsets = []
        pixel_no = 0
        for row_no in range(parameter_table.rowCount()):
            row_data = parameter_table.row(row_no)
            if 'offset' in row_data['Name']:
                pixel_offset = (0.5*self._D7NumberPixelsBank) \
                             - bank_slopes[math.floor(pixel_no / self._D7NumberPixelsBank)] \
                             * (pixel_no % self._D7NumberPixelsBank) - self._RAD_2_DEG * row_data['Value']
                if pixel_no % 2 == 0:
                    pixel_offset -= self._RAD_2_DEG * 0.011 / (2.0 * (1.5177 - 0.01252)) # repeats calculation from the D7 IDF
                pixel_offsets.append(pixel_offset)
                pixel_no += 1
        return wavelength, pixel_offsets

    def _calibrate_detectors(self, parameter_table):
        wavelength, pixel_offsets = self._calculate_pixel_positions(parameter_table)
#         bank2_pixels = pixel_offsets[:44]
#         pos_bank2 = CreateWorkspace(DataX=range(44), DataY=bank2_pixels, NSpec=1)
#         bank3_pixels = pixel_offsets[44:87]
#         pos_bank3 = CreateWorkspace(DataX=range(44, 88), DataY=bank3_pixels, NSpec=1)
#         bank4_pixels = pixel_offsets[89:132]
#         pos_bank4 = CreateWorkspace(DataX=range(89, 132), DataY=bank4_pixels, NSpec=1)

        return wavelength, pixel_offsets

    def _prettify(self, elem):
        """Returns a pretty-printed XML string for the Element."""
        rough_string = ET.tostring(elem, 'utf-8')
        reparsed = minidom.parseString(rough_string)
        return reparsed.toprettyxml(indent="  ")

    def _print_parameter_file(self, wavelength, pixel_offsets):
        """Prints the pixel positions as a Instrument Parameter
        File that can be later used by the D7 loader."""
        param_file = ET.Element('parameter-file')
        param_file.set('instrument', 'D7')
        date_today = date.today()
        param_file.set('date',  str(date_today))

        for bank_no in range(self._D7NumberBanks):
            bank = ET.SubElement(param_file, 'component-link')
            bank.set('name', 'bank'+str(bank_no+2))

            wavelength_information = ET.SubElement(param_file, 'wavelength')
            param = ET.SubElement(wavelength_information, 'parameter')
            param.set('name', 'wavelength')
            value = ET.SubElement(param, 'value')
            value.set('val', str(wavelength))

            for pixel_no in range(self._D7NumberPixelsBank):
                pixel = ET.SubElement(bank, 'parameter')
                pixel.set('name', 'twoTheta_pixel_{0}'.format(pixel_no+1))
                value = ET.SubElement(pixel, 'value')
                value.set('val', str(pixel_offsets[bank_no*self._D7NumberPixelsBank + pixel_no]))

        if self.getProperty("CalibrationFilename").isDefault:
            filename = "d7_{0}.xml".format(date_today)
        else:
            filename = self.getPropertyValue("CalibrationFilename")
        outfile = open(os.path.join(os.getcwd(), filename), "w")
        outfile.write(self._prettify(param_file))


AlgorithmFactory.subscribe(ILLYIGPositionCalibration)
