#pylint: disable=no-init,invalid-name
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import os


def _count_monitors(raw_file):
    """
    Returns the number of monitors and if they're at the start or end of the file
    """

    raw_file = mtd[raw_file]
    num_hist = raw_file.getNumberHistograms()
    detector = raw_file.getDetector(0)
    mon_count = 1

    if detector.isMonitor():
        # Monitors are at the start
        for i in range(1, num_hist):
            detector = raw_file.getDetector(i)

            if detector.isMonitor():
                mon_count += 1
            else:
                break

        return mon_count, True
    else:
        # Monitors are at the end
        detector = raw_file.getDetector(num_hist)

        if not detector.isMonitor():
            #if it's not, we don't have any monitors!
            return 0, True

        for i in range(num_hist, 0, -1):
            detector = raw_file.getDetector(i)

            if detector.isMonitor():
                mon_count += 1
            else:
                break

        return mon_count, False


class TimeSlice(PythonAlgorithm):

    def category(self):
        return 'PythonAlgorithms;Inelastic'


    def summary(self):
        return 'Performa an integration on a raw file over a specified time of flight range'


    def PyInit(self):
        self.declareProperty(StringArrayProperty(name='InputFiles'),
                             doc='Comma separated list of input files')

        self.declareProperty(WorkspaceProperty(name='CalibrationWorkspace', defaultValue='',\
                             direction=Direction.Input, optional=PropertyMode.Optional),
                             doc='Calibration workspace')

        self.declareProperty(IntArrayProperty(name='SpectraRange'),
                             doc='Range of spectra to use')

        self.declareProperty(FloatArrayProperty(name='PeakRange'),
                             doc='Peak range in time of flight')

        self.declareProperty(FloatArrayProperty(name='BackgroundRange'),
                             doc='Background range in time of flight')

        self.declareProperty(name='Plot', defaultValue=False,
                             doc='Plot result workspaces')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save result workspaces as nexus files to default save directory')

        self.declareProperty(name='OutputNameSuffix', defaultValue='_slice',
                             doc='Suffix to append to raw file name for name of output workspace')

        self.declareProperty(WorkspaceGroupProperty(name='OutputWorkspace', defaultValue='',\
                             direction=Direction.Output),
                             doc='Name of workspace group to group result workspaces into')


    def _validate_range(self, name):
        """
        Validates a range property

        @param name Name of the property
        """

        range_prop = self.getProperty(name).value

        if len(range_prop) != 2:
            return 'Range must have two values'

        if range_prop[0] > range_prop[1]:
            return 'Range must be in format "low,high"'

        return ''


    def validateInput(self):
        issues = dict()

        issues['SpectraRange'] = self._validate_range('SpectraRange')
        issues['PeakRange'] = self._validate_range('PeakRange')

        if self.getPropertyValue('BackgroundRange') != '':
            issues['BackgroundRange'] = self._validate_range('BackgroundRange')

        return issues


    def PyExec(self):
        from IndirectCommon import CheckXrange

        self._setup()

        # CheckXrange(xRange, 'Time')
        out_ws_list = []

        for index, filename in enumerate(self._raw_files):
            raw_file = self._read_raw_file(filename)

            # Only need to process the calib file once
            if index == 0 and self._calib_ws is not None:
                self._process_calib(raw_file)

            slice_file = self._process_raw_file(raw_file)
            Transpose(InputWorkspace=slice_file, OutputWorkspace=slice_file)
            unit = mtd[slice_file].getAxis(0).setUnit("Label")
            unit.setLabel("Spectrum Number", "")

            out_ws_list.append(slice_file)
            DeleteWorkspace(raw_file)

            if self._save:
                work_dir = config['defaultsave.directory']
                save_path = os.path.join(work_dir, slice_file + '.nxs')
                SaveNexusProcessed(InputWorkspace=slice_file, Filename=save_path)
                logger.information('Output file :' + save_path)

        all_workspaces = ','.join(out_ws_list)
        GroupWorkspaces(InputWorkspaces=all_workspaces, OutputWorkspace=self._out_ws_group)
        self.setProperty('OutputWorkspace', self._out_ws_group)

        if self._plot:
            try:
                from IndirectImport import import_mantidplot
                mp = import_mantidplot()
                mp.plotSpectrum(slice_file, 0)
            except RuntimeError, e:
                # User clicked cancel on plot so don't do anything
                pass


    def _setup(self):
        """
        Gets properties.
        """

        self._raw_files = self.getProperty('InputFiles').value
        self._spectra_range = self.getProperty('SpectraRange').value
        self._peak_range = self.getProperty('PeakRange').value
        self._output_ws_name_suffix = self.getPropertyValue('OutputNameSuffix')

        self._background_range = self.getProperty('BackgroundRange').value
        if len(self._background_range) == 0:
            self._background_range = None

        self._calib_ws = self.getPropertyValue('CalibrationWorkspace')
        if self._calib_ws == '':
            self._calib_ws = None

        self._out_ws_group = self.getPropertyValue('OutputWorkspace')

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value


    def _read_raw_file(self, filename):
        """
        Loads a raw run file.

        @param filename Data file name
        @returns Name of workspace loaded into
        """

        logger.information('Reading file :' + filename)

        # Load the raw file
        f_name = os.path.split(filename)[1]
        workspace_name = os.path.splitext(f_name)[0]

        Load(Filename=filename, OutputWorkspace=workspace_name, LoadLogFiles=False)

        return workspace_name


    def _process_calib(self, raw_file):
        """
        Run the calibration file with the raw file workspace.

        @param raw_file Name of calibration file
        """

        calib_spec_min = int(self._spectra_range[0])
        calib_spec_max = int(self._spectra_range[1])

        if calib_spec_max - calib_spec_min > mtd[self._calib_ws].getNumberHistograms():
            raise IndexError('Number of spectra used is greater than the number of spectra in the calibration file.')

        # Offset cropping range to account for monitors
        (mon_count, at_start) = _count_monitors(raw_file)

        if at_start:
            calib_spec_min -= mon_count + 1
            calib_spec_max -= mon_count + 1

        # Crop the calibration workspace, excluding the monitors
        CropWorkspace(InputWorkspace=self._calib_ws, OutputWorkspace=self._calib_ws,
                      StartWorkspaceIndex=calib_spec_min, EndWorkspaceIndex=calib_spec_max)


    def _process_raw_file(self, raw_file):
        """
        Process a raw sample file.

        @param raw_file Name of file to process
        """
        from IndirectCommon import CheckHistZero

        # Crop the raw file to use the desired number of spectra
        # less one because CropWorkspace is zero based
        CropWorkspace(InputWorkspace=raw_file, OutputWorkspace=raw_file,
                      StartWorkspaceIndex=int(self._spectra_range[0]) - 1,
                      EndWorkspaceIndex=int(self._spectra_range[1]) - 1)

        num_hist = CheckHistZero(raw_file)[0]

        # Use calibration file if desired
        if self._calib_ws is not None:
            Divide(LHSWorkspace=raw_file, RHSWorkspace=self._calib_ws, OutputWorkspace=raw_file)

        # Construct output workspace name
        run = mtd[raw_file].getRun().getLogData('run_number').value
        slice_file = raw_file[:3].lower() + run + self._output_ws_name_suffix

        if self._background_range is None:
            Integration(InputWorkspace=raw_file, OutputWorkspace=slice_file,
                        RangeLower=self._peak_range[0], RangeUpper=self._peak_range[1],
                        StartWorkspaceIndex=0, EndWorkspaceIndex=num_hist - 1)
        else:
            CalculateFlatBackground(InputWorkspace=raw_file, OutputWorkspace=slice_file,
                                    StartX=self._background_range[0], EndX=self._background_range[1],
                                    Mode='Mean')
            Integration(InputWorkspace=slice_file, OutputWorkspace=slice_file,
                        RangeLower=self._peak_range[0], RangeUpper=self._peak_range[1],
                        StartWorkspaceIndex=0, EndWorkspaceIndex=num_hist - 1)

        return slice_file


AlgorithmFactory.subscribe(TimeSlice)
