# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.simpleapi import *
from mantid.kernel import Direction, FloatBoundedValidator
from mantid.api import FileAction, FileProperty, MatrixWorkspaceProperty, MultipleFileProperty, \
    NumericAxis, Progress, PropertyMode, PythonAlgorithm, WorkspaceProperty
from datetime import date
import math
import numpy as np
import os
from xml.dom import minidom
import xml.etree.ElementTree as ET


class ILLD7YIGPositionCalibration(PythonAlgorithm):

    # helper conversions
    _RAD_2_DEG = 180.0 / np.pi
    _DEG_2_RAD = 1.0 / _RAD_2_DEG
    # number of detectors
    _D7NumberBanks = 3
    _D7NumberPixels = 132
    _D7NumberPixelsBank = 44
    # an approximate universal peak width:
    _peakWidth = 2.0 # in degrees
    _beamMask1 = -35.0
    _beamMask2 = 15.0
    _minDistance = None
    _scanStepSize = 0.25

    def category(self):
        return 'ILL\\Diffraction'

    def summary(self):
        return 'Performs D7 position calibration using YIG scan and returns D7 IPF readable by the NeXus data loader.'

    def seeAlso(self):
        return ['LoadILLPolarizedDiffraction']

    def name(self):
        return 'ILLD7YIGPositionCalibration'

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
                             defaultValue=3.1,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="The initial guess for the neutrons' wavelength")

        self.declareProperty(name="MinimalDistanceBetweenPeaks",
                             defaultValue=3.0,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="The minimal allowable distance between two YIG peaks (in degrees 2theta).")

        self.declareProperty(name="BankOffsets",
                             defaultValue='0,0,0',
                             direction=Direction.Input,
                             doc="List of values of offset for each bank (in degrees).")

        self.declareProperty(name="ScanStepSize",
                             defaultValue=0.25,
                             validator=FloatBoundedValidator(lower=0),
                             direction=Direction.Input,
                             doc="The length of each step during YIG scan (in degrees).")

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
        if not self.getProperty("BankOffsets").isDefault:
            offsets = self.getPropertyValue("BankOffsets").split(',')
            for bank_no in range(self._D7NumberBanks):
                ChangeBinOffset(intensityWS, offsets[bank_no],
                                WorkspaceIndexList='{0}-{1}'.format(bank_no*self._D7NumberPixelsBank,
                                                                    (bank_no+1)*self._D7NumberPixelsBank-1),
                                OutputWorkspace=intensityWS)

        if not self.getProperty("ScanStepSize").isDefault:
            self._scanStepSize = float(self.getProperty("ScanStepSize")[0])

        self._minDistance = float(self.getPropertyValue("MinimalDistanceBetweenPeaks"))

        # load the YIG peaks from an IPF
        yig_d = self._load_yig_peaks(intensityWS)
        # check how many and which peaks can be fitted in each row
        yig_peaks_positions = self._get_yig_peaks_positions(intensityWS, yig_d)
        progress.report()
        # fit gaussian to peaks for each pixel, returns peaks as a function of their expected position
        peaks_positions = self._fit_bragg_peaks(intensityWS, yig_peaks_positions)
        progress.report()
        ReplaceSpecialValues(InputWorkspace='fit_results', OutputWorkspace='fit_results',
                             NaNValue=0, NaNError=0, InfinityValue=0, InfinityError=0, checkErrorAxis=True)
        # fit the wavelegnth, bank gradients and individual
        detector_parameters = self._fit_detector_positions(peaks_positions)
        progress.report()
        # calculates pixel positions, bank offsets and slopes from the fit output
        wavelength, pixel_offsets, bank_offsets, bank_slopes = self._calculate_pixel_positions(detector_parameters)
        progress.report()
        # print the Instrument Parameter File that can be used in the ILLPolarizedDiffraction loader
        self._print_parameter_file(wavelength, pixel_offsets, bank_offsets, bank_slopes)
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
            # normalize to monitor1 as monitor2 is sometimes empty:
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
            workspace = MaskBinsIf(workspace, Criterion='x>{0} && x<{1}'.format(self._beamMask1, self._beamMask2))
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
        ClearInstrumentParameters(ws) #in case other IPF was loaded there before
        LoadParameterFile(Workspace=ws, Filename=parameterFilename)
        yig_d_set = set()
        instrument = ws.getInstrument().getComponentByName('detector')
        for param_name in instrument.getParameterNames(True):
            if 'peak_' in param_name:
                yig_d_set.add(instrument.getNumberParameter(param_name)[0])
        return sorted(list(yig_d_set))

    def _remove_unwanted_yig_peaks(self, yig_list):
        """Removes YIG peaks with theta above 180 degrees
        and those that are too close to each other,
        and returns a list with peaks positions in 2theta"""
        wavelength = float(self.getPropertyValue("ApproximateWavelength"))
        yig_list = [2.0 * self._RAD_2_DEG * math.asin(wavelength / (2.0 * d_spacing))
                    for d_spacing in yig_list if d_spacing > 0.5*wavelength]
        yig_peaks = []
        for index in range(len(yig_list)-1):
            if abs(yig_list[index]-yig_list[index+1]) >= 2*self._minDistance:
                yig_peaks.append(yig_list[index])
        return self._include_other_quadrants(yig_peaks)

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
        yig2theta = self._remove_unwanted_yig_peaks(YIG_D)
        peak_list = []
        for pixel_no in range(ws.getNumberHistograms()):
            detector_2theta = ws.readX(pixel_no)
            intensity = ws.readY(pixel_no)
            min2Theta = detector_2theta[0]
            max2Theta = detector_2theta[-1]
            single_spectrum_peaks = []
            for peak in yig2theta:
                if ( ((peak - self._peakWidth*2) > min2Theta ) and ( (peak + self._peakWidth*2) < max2Theta)
                     and (peak+self._peakWidth*2 < self._beamMask1 or peak-self._peakWidth*2 > self._beamMask2)):
                    peak_intensity = 0
                    for bin_no in range(len(detector_2theta)):
                        twoTheta = detector_2theta[bin_no]
                        if (abs(twoTheta-peak) < 1): # within 1 degree from the expected peak position
                            # scan for the local maximum:
                            peak_intensity = intensity[bin_no]
                            index_maximum = bin_no
                            for index in range(bin_no-int((self._minDistance)/self._scanStepSize),
                                               bin_no+int((self._minDistance)/self._scanStepSize), 1):
                                if intensity[index] > peak_intensity:
                                    peak_intensity = intensity[index]
                                    index_maximum = index
                            expected_pos = peak
                            single_spectrum_peaks.append((peak_intensity, detector_2theta[index_maximum], expected_pos))
                            break
            peak_list.append(single_spectrum_peaks)
        return peak_list

    def _fit_bragg_peaks(self, ws, yig_peaks):
        """ Fits peaks defined in the yig_peaks argument
        returns a workspace with fitted peak positions
        on the Y axis and the expected positions on the X axis"""
        max_n_peaks = len(max(yig_peaks, key=len))
        ws_names = []
        for pixel_no in range(ws.getNumberHistograms()):
            # create the needed columns in the output workspace
            results_x = np.zeros(max_n_peaks)
            results_y = np.zeros(max_n_peaks)
            results_e = np.zeros(max_n_peaks)
            single_spectrum_peaks = yig_peaks[pixel_no]
            if len(single_spectrum_peaks) >= 1:
                peak_no = 0
                for peak_intensity, peak_centre_guess, peak_centre_expected in single_spectrum_peaks:
                    function = "name=LinearBackground, A0=1e-4, A1=1e-4;\n"
                    function += "name=Gaussian, PeakCentre={0}, Height={1}, Sigma={2};\n".format(
                                                                                                peak_centre_guess,
                                                                                                peak_intensity,
                                                                                                0.5*self._peakWidth)
                    constraints = "f1.Height > 0, f1.Sigma < {0}".format(0.75*self._peakWidth)
                    constraints += ",{0} < f1.PeakCentre < {1}".format(float(peak_centre_guess) - self._minDistance,
                                                                       float(peak_centre_guess) + self._minDistance)
                    ws_name = 'pixel_{0}_peak_{1}'.format(pixel_no, peak_no)
                    ws_names.append(ws_name)
                    fit_output = Fit(Function=function,
                                     InputWorkspace=ws,
                                     WorkspaceIndex=pixel_no,
                                     StartX=float(peak_centre_expected) - self._minDistance,
                                     EndX=float(peak_centre_expected) + self._minDistance,
                                     Constraints=constraints,
                                     CreateOutput=True,
                                     IgnoreInvalidData=True,
                                     Output='out')
                    RenameWorkspace(InputWorkspace='out_Workspace', OutputWorkspace=ws_name)
                    param_table = fit_output.OutputParameters
                    try:
                        param_table
                        rowCount = param_table.rowCount()
                    except NameError:
                        rowCount = 0
                    for row_no in range(rowCount):
                        row_data = param_table.row(row_no)
                        if 'A0' in row_data['Name']:
                            background = row_data['Value']
                        if 'PeakCentre' in row_data['Name']:
                            intensity, peak_pos_guess, peak_pos_expected = single_spectrum_peaks[peak_no]
                            if (abs(param_table.row(row_no-1)['Value'] / background) > 0.1):
                                results_x[peak_no] = peak_pos_expected
                                results_y[peak_no] = row_data['Value']
                                results_e[peak_no] = row_data['Error']
                    peak_no += 1
            try:
                fit_results
            except NameError:
                fit_results = CreateWorkspace(DataX=results_x,
                                              DataY=results_y,
                                              DataE=results_e,
                                              UnitX='degrees',
                                              NSpec=1)
            else:
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

        GroupWorkspaces(ws_names, OutputWorkspace='single_peaks_fits')

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
        ties_bank_off_list = []
        ties_gradient = ""
        ties_bank_off = ""
        pixel_offset_constr = self._DEG_2_RAD*25 # +-25 degrees
        bank_offset_constr = self._DEG_2_RAD*5 # +-5 degrees around the bank position
        gradient_constr = 0.5 # +-5% around the m = 1.0 value
        lambda_constr = 0.5 # +- 5% of lambda variation from the initial assumption
        constraint_list = ['{0}<f0.lambda<{1}'.format(1-lambda_constr, 1+lambda_constr)]

        for pixel_no in range(ws.getNumberHistograms()):
            function = 'name=UserFunction, \
            Formula = {0} * m * ( 2.0 * asin( lambda * sin( 0.5 * {1} * x ) ) + offset+ bank_offset), \
            lambda= 1.0, m = 1.0, offset = {2}, bank_offset = {3}, $domains=i'.format(self._RAD_2_DEG, self._DEG_2_RAD, 0, 0)
            function_list.append(function)
            constraint_list.append('-{0} < f{1}.offset < {0}'.format(pixel_offset_constr,
                                                                     pixel_no))
            constraint_list.append('-{0} < f{1}.bank_offset < {0}'.format(bank_offset_constr,
                                                                          pixel_no))
            constraint_list.append('{0} < f{1}.m < {2}'.format(1-gradient_constr,
                                                               pixel_no,
                                                               1+gradient_constr))
            # add a global tie on lambda:
            ties_lambda_list.append('f{0}.lambda'.format(pixel_no))
            # set ties for three bank gradients:
            ties_gradient_list.append('f{0}.m'.format(pixel_no))
            ties_bank_off_list.append('f{0}.bank_offset'.format(pixel_no))
            if pixel_no % self._D7NumberPixelsBank == (self._D7NumberPixelsBank-1):
                ties_gradient += ',' + '='.join(ties_gradient_list)
                ties_bank_off += ',' + '='.join(ties_bank_off_list)
                ties_gradient_list=[]
                ties_bank_off_list=[]

        ties_lambda = '='.join(ties_lambda_list)
        ties = ties_lambda + ties_gradient + ties_bank_off

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
        try:
            fit_output = Fit(Function=multiFunc,
                             Constraints=constraints,
                             IgnoreInvalidData=True,
                             CreateOutput=True,
                             Output='det_out',
                             **fit_kwargs)
        except RuntimeError:
            print("Please change initial parameters of the fit")
            raise
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
        bank2_offset = self._RAD_2_DEG * float(parameter_table.column(1)[3])
        if parameter_table.column(1)[3*self._D7NumberPixelsBank] == 0:
            raise RuntimeError('Bank3 slope is equal to 0.')
        bank3_slope = 1.0 / float(parameter_table.column(1)[4*self._D7NumberPixelsBank])
        bank3_offset = self._RAD_2_DEG * float(parameter_table.column(1)[4*self._D7NumberPixelsBank+3])
        if parameter_table.column(1)[6*self._D7NumberPixelsBank] == 0:
            raise RuntimeError('Bank4 slope is equal to 0.')
        bank4_slope = 1.0 / float(parameter_table.column(1)[8*self._D7NumberPixelsBank])
        bank4_offset = self._RAD_2_DEG * float(parameter_table.column(1)[8*self._D7NumberPixelsBank+3])
        bank_slopes = [bank2_slope, bank3_slope, bank4_slope]
        bank_offsets = [bank2_offset, bank3_offset, bank4_offset]
        user_offsets = self.getPropertyValue("BankOffsets").split(',')
        bank_offsets = [offset1+float(offset2) for offset1, offset2 in zip(bank_offsets, user_offsets)]
        pixel_offsets = []
        pixel_no = 0
        for row_no in range(parameter_table.rowCount()):
            row_data = parameter_table.row(row_no)
            if '.offset' in row_data['Name']:
                pixel_offset = self._RAD_2_DEG * row_data['Value'] \
                             + (0.5*self._D7NumberPixelsBank) \
                             - bank_slopes[math.floor(pixel_no / self._D7NumberPixelsBank)] * (pixel_no % self._D7NumberPixelsBank)
                if pixel_no % 2 == 0:
                    pixel_offset -= self._RAD_2_DEG * 0.011 / (2.0 * (1.5177 - 0.01252)) # repeats calculation from the D7 IDF
                pixel_offsets.append(pixel_offset)
                pixel_no += 1
        return wavelength, pixel_offsets, bank_offsets, bank_slopes

    def _prettify(self, elem):
        """Returns a pretty-printed XML string for the Element."""
        rough_string = ET.tostring(elem, 'utf-8')
        reparsed = minidom.parseString(rough_string)
        return reparsed.toprettyxml(indent="  ")

    def _print_parameter_file(self, wavelength, pixel_offsets, bank_offsets, bank_slopes):
        """Prints the pixel positions as a Instrument Parameter
        File that can be later used by the D7 loader."""
        param_file = ET.Element('parameter-file')
        param_file.set('instrument', 'D7')
        date_today = date.today()
        param_file.set('date',  str(date_today))

        # include the fitted wavelength in the output IPF
        detector = ET.SubElement(param_file, 'component-link')
        detector.set('name', 'detector')
        param = ET.SubElement(detector, 'parameter')
        param.set('name', 'wavelength')
        value = ET.SubElement(param, 'value')
        value.set('val', str(wavelength))

        for bank_no in range(self._D7NumberBanks):
            bank = ET.SubElement(param_file, 'component-link')
            bank.set('name', 'bank'+str(bank_no+2))

            offset = ET.SubElement(bank, 'parameter')
            offset.set('name', 'offset')
            value = ET.SubElement(offset, 'value')
            value.set('val', str(bank_offsets[bank_no]))

            gradient = ET.SubElement(bank, 'parameter')
            gradient.set('name', 'gradient')
            value = ET.SubElement(gradient, 'value')
            value.set('val', str(bank_slopes[bank_no]))

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
        outfile.close()


AlgorithmFactory.subscribe(ILLD7YIGPositionCalibration)
