# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import re
import math
import numpy as np
import h5py
import json

from configparser import ConfigParser, NoOptionError, NoSectionError

from mantid import mtd
from mantid.kernel import (StringListValidator, Direction, DateAndTime,
                           StringArrayMandatoryValidator, StringArrayProperty, CompositeValidator)
from mantid.api import (PythonAlgorithm, FileProperty, WorkspaceProperty, FileAction,
                        Progress)
from mantid.simpleapi import *  # noqa


def range_to_values(rng):

    return tuple([float(x) for x in rng.split(',')])


def cycle_and_runs(run_seq):
    ss = run_seq.split('::')
    if len(ss) == 2:
        return int(ss[0]), ss[1]
    else:
        return None, ss[0]


def seq_to_list(iseqn):
    # convert a comma separated range of numbers returned as a list
    # first clean all whitespaces
    seqn = iseqn.replace(' ', '')
    nlist = []
    sqlist = seqn.split(',')
    for run in sqlist:
        if run == '':
            continue
        ss = run.split('-')
        try:
            run_start = int(ss[0])
            run_end = int(ss[-1])
            if len(ss) == 1:
                nlist.append(run_start)
            else:
                for i in range(run_start, run_end+1):
                    nlist.append(i)
        except ValueError:
            raise RuntimeError('Unexpected run sequence: {}'.format(seqn))

    return nlist


def dataset_seq_to_list(iseqn):
    # uses a ';' as a separator and expects
    # 'run_seqn:data_sets;...'
    # returns a list [path:dset,...]
    run_list = []
    dataset_list = []
    for seq in iseqn.split(';'):
        # check for data set spec else use dataset 0
        ss = seq.split(':')
        runs = ss[0]
        datasets = seq_to_list(ss[1]) if len(ss) > 1 else [0]
        for run in seq_to_list(runs):
            for dataset in datasets:
                run_list.append(run)
                dataset_list.append(dataset)

    return run_list, dataset_list


def list_to_seq(nlist):

    # converts a list of numbers into an ordered string sequence,
    # for example [1,2,3,10,7,6,8] -> '1-3,6-8,10'

    sequence = []
    # rewrite using range notation to a simple sequence
    sorted_runs = sorted(nlist)
    run_end, run_start = sorted_runs[0], sorted_runs[0]
    for next_run in sorted_runs[1:]:
        if next_run == run_end + 1:
            run_end = next_run
        else:
            # add the range
            if run_start != run_end:
                sequence.append(str(run_start) + '-' + str(run_end))
            else:
                sequence.append(str(run_start))
            run_start, run_end = next_run, next_run
    if run_start != next_run:
        sequence.append(str(run_start) + '-' + str(next_run))
    else:
        sequence.append(str(next_run))
    return sequence


def extract_runs(file_list, collapse=False):

    # extracts the 7 digit number in the basename removing initial zeros
    seq = []
    for fpath in file_list:
        base = os.path.basename(fpath)
        s = re.findall(r'[0-9]{7}', base)
        if len(s) > 0:
            seq.append(s[0].lstrip('0'))

    if collapse and len(seq) > 1:
        return list_to_seq([int(s) for s in seq])
    else:
        return seq


def build_file_list(file_prefix, file_extn, runs):

    # all the files include a 7 digit run number embedded in the file name
    data_runs, run_index = dataset_seq_to_list(runs)
    data_files = []
    for run, index in zip(data_runs, run_index):
        data_files.append(f"{file_prefix}{run:07d}{file_extn}:{index}")

    # remove any repeated files and sort
    data_files = sorted(list(set(data_files)))

    return data_files


def split_run_index(run):
    # splits fpath.hdf:n to fpath.hdf and n
    try:
        gp = re.search(r':([0-9]+?)$', run)
        index = int(gp.group(1))
        base = run[:gp.start()]
    except AttributeError:
        index = 0
        base = run
    return base, index


def extract_hdf_params(fpath, tags):

    # gets the parameters from the base file to be able to complete the setup
    # such as, doppler amplitude and speed
    # if the hdf parameter is mssing and no default is provide an
    # exception is raised

    values = {}
    with h5py.File(fpath, 'r') as fp:
        for key, hdf_tag, def_value in tags:
            try:
                values[key] = fp[hdf_tag][()]
            except KeyError:
                if def_value is None:
                    raise RuntimeError('Missing {} in {}'.format(key, fpath))
                values[key] = def_value
    return values


def find_file(path_list, fname):
    # prioritise the path_list and then the default locations
    for path in path_list:
        fpath = os.path.normpath(os.path.join(path, fname))
        if os.path.isfile(fpath):
            return fpath
    return FileFinder.getFullPath(fname)


def find_event_path(spath, evpath):
    # if the hdf source path is in the data cycle subdirectory
    # then insert the folder at the start of the search folders
    # and add the adjacent cycles to the search options
    cycle_path = []
    sfolder, file = os.path.split(spath)
    gp = re.search(r'data[\\/]cycle[\\/]([0-9]+?)$', sfolder)
    if gp:
        cycle = int(gp.group(1))
        cycle_path = [sfolder]
        for c in [cycle-1, cycle+1]:
            cycle_path.append(
                re.sub(r'cycle[\\/][0-9]{3}', 'cycle/{:03d}'.format(c), sfolder))

    # get the subfolder reference for the event file
    tags = [('evfolder', '/entry1/instrument/detector/daq_dirname', None)]
    value = extract_hdf_params(spath, tags)['evfolder'][0]
    evfolder = value.decode('ascii') if value else '.'

    # run through the event path list to find the sub folder
    for ep in evpath:
        if os.path.isabs(ep):
            tpath = os.path.join(ep, evfolder)
            if os.path.isdir(tpath):
                return ep
        else:
            for cp in cycle_path:
                tpath = os.path.join(cp, ep, evfolder)
                if os.path.isdir(tpath):
                    return os.path.normpath(os.path.join(cp, ep))

    # gets to here if it cannot find the event file location
    # just try current directory
    return './'


def beam_monitor_counts(src):
    run = mtd[src].getRun()
    return np.sum(run.getProperty('MonitorCounts').value)


def scale_and_remove_background(source_ws, source_scale, empty_ws, empty_scale,
                                out_ws, floor_negatives):
    # Common reduction processing step in the algorithm,
    #   source_scale.source_ws - empty_scale.empty_ws -> out_ws
    alpha_scale = source_scale * 1.0e6 / beam_monitor_counts(source_ws)
    Scale(InputWorkspace=source_ws,
            Factor=alpha_scale, OutputWorkspace=out_ws)
    if empty_ws is not None:
        factor = empty_scale * 1.0e6 / beam_monitor_counts(empty_ws)
        Scale(InputWorkspace=empty_ws, Factor=factor,
                OutputWorkspace=empty_ws)
        Minus(LHSWorkspace=out_ws, RHSWorkspace=empty_ws,
                OutputWorkspace=out_ws)
        if floor_negatives:
            ResetNegatives(InputWorkspace=out_ws,
                            OutputWorkspace=out_ws, AddMinimum=False)


class PelicanReduction(PythonAlgorithm):

    def category(self):
        return "Workflow\\Inelastic;Inelastic;Inelastic\\Reduction"

    def summary(self):
        return 'Performs an inelastic energy transfer reduction for ANSTO Pelican geometry data.'

    def seeAlso(self):
        return []

    def name(self):
        return "PelicanReduction"

    def PyInit(self):

        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        self.declareProperty(StringArrayProperty('SampleRuns',
                                                 values=[],
                                                 validator=mandatoryInputRuns),
                             doc='Optional cycle number followed by comma separated range of\n'
                                 'sample runs as [cycle::] n1,n2,..\n'
                                 ' eg 123::7333-7341,7345')

        self.declareProperty(name='EmptyRuns',
                             defaultValue='',
                             doc='Optional cycle number followed by comma separated range of\n'
                                 'runs as [cycle::] n1,n2,..\n'
                                 '  eg 123::6300-6308')

        self.declareProperty(name='ScaleEmptyRuns',
                             defaultValue=1.0,
                             doc='Scale the empty runs prior to subtraction')

        self.declareProperty(name='CalibrationRuns',
                             defaultValue='',
                             doc='Optional cycle number followed by comma separated range of\n'
                                 'runs as [cycle::] n1,n2,..\n'
                                 '  eg 123::6350-6365')

        self.declareProperty(name='EmptyCalibrationRuns',
                             defaultValue='',
                             doc='Optional cycle number followed by comma separated range of\n'
                                 'runs as [cycle::] n1,n2,..\n'
                                 '  eg 123::6370-6375')

        self.declareProperty(name='EnergyTransfer',
                             defaultValue='0.0, 0.02, 3.0',
                             doc='Energy transfer range in meV expressed as min, step, max')

        self.declareProperty(name='MomentumTransfer',
                             defaultValue='0.0, 0.02, 2.6',
                             doc='Momentum transfer range in inverse Angstroms,\n'
                                 ' expressed as min, step, max')

        self.declareProperty(name='Processing', defaultValue='SOFQW1-Centre',
                             validator=StringListValidator(
                                 ['SOFQW1-Centre', 'SOFQW3-NormalisedPolygon', 'NXSPE']),
                             doc='Convert to SOFQW or save file as NXSPE,\n'
                                 'note SOFQW3 is more accurate but much slower than SOFQW1.')

        self.declareProperty(name='LambdaOnTwoMode', defaultValue=False,
                             doc='Set if instrument running in lambda on two mode.')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Name for the reduced workspace.')

        self.declareProperty(FileProperty('ScratchFolder', '',
                                          action=FileAction.OptionalDirectory,
                                          direction=Direction.Input),
                             doc='Path to save and restore merged workspaces.')

        self.declareProperty(name='KeepIntermediateWorkspaces', defaultValue=False,
                             doc='Whether to keep the intermediate sample and calibration\n'
                                 'workspaces for diagnostic checks.')

        self.declareProperty(FileProperty('ConfigurationFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['ini']),
                             doc='Optional: INI file to override default processing values.')

    def PyExec(self):

        # Set up the processing parameters
        self.setUp()

        # Get the list of data files from the runs
        sample_runs = self._hdf_files_from_runs('SampleRuns')
        empty_runs = self._hdf_files_from_runs('EmptyRuns')
        calibration_runs = self._hdf_files_from_runs('CalibrationRuns')
        empty_calib_runs = self._hdf_files_from_runs('EmptyCalibrationRuns')
        total_runs = sum([len(x) for x in [sample_runs, empty_runs, calibration_runs,
                                           empty_calib_runs]])

        # set up the wavelength from the sample runs - sample runs includes the dataset index
        # which needs to be removed
        sample_file = re.sub(r':[0-9]+$', '', sample_runs[0])
        self.set_efixed(sample_file)

        # The progress includes 4 additional status reports on top of incrementing
        # the progress on each loaded file
        self._progress = Progress(
            self, start=0.0, end=1.0, nreports=total_runs + 4)
        self._progress.report('File selection complete, loading initial file')

        # If the output workspace is not provided use the basename of the first
        # sample file
        output_ws = self.getPropertyValue('OutputWorkspace')

        # load, merge and convert to the app. energy units
        # the calibration uses a fixed energy bin range to ensure consistent
        # integration results with the FindEPP algorithm
        cal_energy_bins = '-1.5, 0.02, 1.5'
        _sample_ws = self._load_and_reduce('_sample', sample_runs)
        _empty_ws = self._load_and_reduce('_empty', empty_runs)
        _calibration_ws = self._load_and_reduce(
            '_calibration', calibration_runs, energy_bins=cal_energy_bins)
        _empty_calib_ws = self._load_and_reduce(
            '_empty_calib', empty_calib_runs, energy_bins=cal_energy_bins)

        self._progress.report('Background removal, normalization and cleanup')

        # append the selected processing option to the output ws name
        output_ws += self._process_suffix[self._processing]

        # Perform background removal and normalization against the integrated calibration data as
        #   red_2D = (alpha.sample_ws - empty_ws) / integrated (beta.calibration_ws - empty_cal_ws)
        red_2D = output_ws+'_2D'
        scale_and_remove_background(_sample_ws, self._sample_scale,
                                    _empty_ws, self._scale_empty, red_2D, self._reset_negatives)
        if _calibration_ws is not None:
            denom_ws = '_integ_cal'
            self._integrated_calibration(
                _calibration_ws, _empty_calib_ws, denom_ws)

            # perform normalization step and add the denom_ws to be cleaned up later
            Divide(LHSWorkspace=red_2D, RHSWorkspace=denom_ws,
                   OutputWorkspace=red_2D)
            ReplaceSpecialValues(InputWorkspace=red_2D, OutputWorkspace=red_2D,
                                 NaNValue=0.0, InfinityValue=0.0)
            self._intermediate_ws.append(denom_ws)

        if self._processing == 'NXSPE':
            self._nxspe_processing(red_2D)
        else:
            self._progress.report(
                'SOFQW-{} processing'.format(self._sofqw_mode))
            self._sofqw_processing(red_2D, output_ws)

        # clean up the intermediate workspaces else group them to keep display clean
        if not self._keep_intermediate:
            for wsn in self._intermediate_ws:
                try:
                    DeleteWorkspace(Workspace=wsn)
                except ValueError:
                    pass
            self._intermediate_ws = []
        else:
            GroupWorkspaces(InputWorkspaces=self._intermediate_ws,
                            OutputWorkspace='intermediate')

        self._progress.report('Clean up complete')

    def _nxspe_processing(self, reduced_2D):
        if self._temp_folder is None:
            nxspe_file = reduced_2D + '.nxspe'
        else:
            nxspe_file = os.path.join(self._temp_folder, reduced_2D + '.nxspe')

        # SaveNXSPE works with the detectors only
        red_det = reduced_2D + 'det'
        ExtractMonitors(InputWorkspace=reduced_2D, DetectorWorkspace=red_det)
        SaveNXSPE(InputWorkspace=red_det, Filename=nxspe_file,
                  EFixed=self._efixed, Psi=self._mscor)
        self.setProperty('OutputWorkspace', reduced_2D)
        DeleteWorkspace(Workspace=red_det)

    def _sofqw_processing(self, reduced_2D, output_ws):
        # convert to SofQW and KIKf correction and transpose axis
        SofQW(InputWorkspace=reduced_2D, OutputWorkspace=reduced_2D,
              QAxisBinning=self._q_range, EMode='Direct', EFixed=self._efixed,
              Method=self._sofqw_mode, ReplaceNANs=True)
        CorrectKiKf(InputWorkspace=reduced_2D, OutputWorkspace=reduced_2D,
                    EMode='Direct', EFixed=self._efixed)

        red_1D = output_ws + '_1D_dE'
        SumSpectra(InputWorkspace=reduced_2D, OutputWorkspace=red_1D,
                   RemoveSpecialValues=True)
        Transpose(InputWorkspace=reduced_2D, OutputWorkspace=reduced_2D)

        # generate the 1D results and group with the 2D data and update the axis for the 2D data
        # which was created when the detector grouping was created and add the ini params
        # for completeness
        red_1DQ = output_ws + '_1D_Q'
        # drop exception handling when updated formal Mantid build includes SumSpectra change
        try:
            SumSpectra(InputWorkspace=reduced_2D, OutputWorkspace=red_1DQ,
                       UseFractionalArea=False, RemoveSpecialValues=True)
        except:
            SumSpectra(InputWorkspace=reduced_2D,
                       OutputWorkspace=red_1DQ, RemoveSpecialValues=True)
        self._append_ini_params(red_1D)
        self._append_ini_params(red_1DQ)
        self._append_ini_params(reduced_2D)
        grouped = [red_1D, red_1DQ, reduced_2D]
        GroupWorkspaces(InputWorkspaces=grouped, OutputWorkspace=output_ws)
        self.setProperty('OutputWorkspace', output_ws)

    def _integrated_calibration(self, calibration_ws, empty_calib_ws, output_ws):

        scale_and_remove_background(calibration_ws, self._calibration_scale,
                                    empty_calib_ws, self._cal_background_scale, output_ws,
                                    self._reset_negatives)

        # Scale the data by the calibration after it has been integrated
        if self._cal_peak_intensity:
            self._integrate_over_peak(output_ws, output_ws)
        else:
            Integration(InputWorkspace=output_ws, OutputWorkspace=output_ws,
                        RangeLower=self._lo_integ_range, RangeUpper=self._hi_integ_range)

        # average the normalization over the tube
        self._average_over_tube(output_ws, output_ws)

    def _append_ini_params(self, output_ws):

        # all the options are under a 'processing' section
        run = mtd[output_ws].getRun()
        try:
            options = dict(self._config.items('processing'))
            skeys = sorted(options.keys())
            for k in skeys:
                run.addProperty('ini_' + k, options[k], True)
        except NoSectionError:
            # no file or valid parameters
            pass

    def _get_param(self, ftype, section, option, default):
        try:
            if ftype == bool:
                value = self._config.getboolean(section, option)
            else:
                value = ftype(self._config.get(section, option))
        except (NoOptionError, NoSectionError):
            value = default
        return value

    def set_efixed(self, sample_path):
        # the instrument offers a lambda on two mode which effectively
        # halves the neutron wavelength, the captured raw data stores the
        # nominal wavelength so it needs to be divided by 2
        tags = [('wavelength', '/entry1/instrument/crystal/wavelength', None),
                ('mscor', '/entry1/sample/mscor', 0.0)]
        values = extract_hdf_params(sample_path, tags)
        if self._lambda_on_two:
            wavelength = 0.5 * values['wavelength'][0]
        else:
            wavelength = values['wavelength'][0]
        # standard conversion factor from wavelength (A) to meV using
        # planck's constant and neutron mass
        ANGSTROMS_TO_MEV = 81.804
        self._efixed = float(ANGSTROMS_TO_MEV / wavelength**2)
        self._mscor = float(values['mscor'][0])

    def setUp(self):

        self._pixels_per_tube = 64
        self._detector_spectra = 12800  # 200 * 64
        self._file_prefix = 'PLN'

        self._efixed = None
        self._mscor = None

        # Update the default processing parameters

        # convert to SOFQW or save as NXSPE
        processing = self.getPropertyValue('Processing').split('-')
        self._processing = processing[0]
        self._sofqw_mode = processing[1] if len(processing) > 1 else ''
        self._process_suffix = {'SOFQW1': '_qw1',
                                'SOFQW3': '_qw3',
                                'NXSPE':  '_spe'}

        # from the configuration (ini) file and then
        # from the run properties for the sample
        #
        temp_folder = self.getPropertyValue('ScratchFolder')
        self._temp_folder = None if temp_folder == '' else temp_folder

        self._config = ConfigParser(allow_no_value=True)
        ini_file = self.getPropertyValue('ConfigurationFile')
        if not os.path.isfile(ini_file):
            ini_file = FileFinder.getFullPath(ini_file)
        self._config.read(ini_file)

        self._file_extn = self._get_param(
            str, 'processing', 'file_extn', '.nx.hdf')
        self._analyse_tubes = self._get_param(
            str, 'processing', 'analyse_tubes', '0-199')

        self._ev_range = self.getProperty('EnergyTransfer').value
        self._q_range = self.getProperty('MomentumTransfer').value
        self._scale_empty = self.getProperty('ScaleEmptyRuns').value

        self._data_cycle_path = self._get_param(
            str, 'processing', 'data_cycle_path', '.').strip()
        self._hdf_search_path = [x.strip() for x in self._get_param(
            str, 'processing', 'hdf_search_path', '.').split(';')]
        self._event_search_path = [x.strip() for x in self._get_param(
            str, 'processing', 'event_search_path', './hsdata').split(';')]

        self._cal_peak_intensity = self._get_param(
            bool, 'processing', 'integrate_over_peak', False)
        self._average_peak_width = self._get_param(
            bool, 'processing', 'average_peak_width', False)
        pixels = self._get_param(
            str, 'processing', 'active_pixels', '0-63').split('-')
        self._pixel_range = (int(pixels[0]), int(pixels[-1]))

        self._sample_scale = self._get_param(
            float, 'processing', 'sample_scale', 1.0)
        self._calibration_scale = self._get_param(
            float, 'processing', 'calibration_scale', 1.0)
        self._cal_background_scale = self._get_param(
            float, 'processing', 'cal_background_scale', 1.0)

        self._keep_intermediate = self.getProperty(
            'KeepIntermediateWorkspaces').value
        self._lambda_on_two = self.getProperty('LambdaOnTwoMode').value

        self._lo_integ_range = self._get_param(
            float, 'processing', 'lo_integ_range', 3500.0)
        self._hi_integ_range = self._get_param(
            float, 'processing', 'hi_integ_range', 3900.0)
        self._reset_negatives = self._get_param(
            bool, 'processing', 'reset_negatives', False)

        self._tof_correction = self._get_param(
            float, 'processing', 'tof_correction', 0.0)
        self._max_energy_gain = self._get_param(
            float, 'processing', 'max_energy_gain', 0.0)
        self._calibrate_tof = self._get_param(
            bool, 'processing', 'calibrate_tof', False)

        # set up the loader options used in the scan and reduce
        self._analyse_load_opts = {'BinaryEventPath': './hsdata',
                                   'CalibrateTOFBias': self._calibrate_tof,
                                   'TimeOfFlightBias': self._tof_correction,
                                   'LambdaOnTwoMode': self._lambda_on_two
                                   }

        # keep pre-reduced to avoid rebinning which is slow with the reference
        # data that has a lot of events
        self._intermediate_ws = []

    def _get_loader_options(self, lname, lversion, lopts):
        # create the loader and extract the relevant keys
        alg = self.createChildAlgorithm(lname, lversion)
        alg.initialize()
        opts = {}
        for key in alg.keys():
            if key in lopts and lopts[key] is not None:
                opts[key] = lopts[key]
        return opts

    def _hdf_files_from_runs(self, runs_tag):
        # the run format is cycle:: runs; cycle:: runs; ..
        # to collect all the data the split sequence ';', '::'
        #
        all_runs = self.getPropertyValue(runs_tag)

        # split by cycle first
        analyse_runs = []
        for run_seq in all_runs.split(';'):
            cycle, runs = cycle_and_runs(run_seq)

            # define the search path
            if cycle:
                search_path = [os.path.join(
                    self._data_cycle_path, '{:03d}'.format(cycle))] + self._hdf_search_path
            else:
                search_path = self._hdf_search_path

            # get the list of filenames
            filenames = build_file_list(
                self._file_prefix, self._file_extn, runs)

            # now find the path to all the files
            for file in filenames:
                ss = file.split(':')
                fname = ss[0]
                index = ss[1]
                file_path = find_file(search_path, fname)
                if file_path:
                    analyse_runs.append('{}:{}'.format(file_path, index))
                else:
                    raise RuntimeError('Cannot find file: {}'.format(file))
        return analyse_runs

    def _integrate_over_peak(self, input_ws, output_ws):
        # performs an 3 sigma integration around a gaussian fitted peak
        iws = mtd[input_ws]
        nhist = iws.getNumberHistograms()

        # get the gaussian fit parameters per spectra
        epps = FindEPP(iws)
        lo_vals = np.empty(nhist)
        hi_vals = np.empty(nhist)
        for i in range(nhist):
            peak = epps.cell('PeakCentre', i)
            sigma = epps.cell('Sigma', i)
            lo_vals[i] = peak - 3 * sigma
            hi_vals[i] = peak + 3 * sigma
        DeleteWorkspace(Workspace=epps)

        if self._average_peak_width:
            lo = lo_vals[np.nonzero(lo_vals)].mean()
            hi = hi_vals[np.nonzero(hi_vals)].mean()
            Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws,
                        RangeLower=lo, RangeUpper=hi)
        else:
            Integration(InputWorkspace=input_ws, OutputWorkspace=output_ws,
                        RangeLowerList=lo_vals, RangeUpperList=hi_vals)

    def _average_over_tube(self, input_ws, output_ws):

        # build the vector of tube averaged spectra weighting but ignore the
        # monitors
        ws = mtd[input_ws]
        yv = ws.extractY()
        yd = yv[:self._detector_spectra]
        y2d = yd.reshape(-1, self._pixels_per_tube)
        yk = np.ones_like(y2d).T * np.mean(y2d, axis=1)
        yav = yk.T.reshape(-1)

        # create the output workspace and replace the Y values
        if output_ws != input_ws:
            CloneWorkspace(InputWorkspace=input_ws, OutputWorkspace=output_ws)
        ows = mtd[output_ws]
        for i in range(len(yav)):
            ows.dataY(i)[0] = yav[i] if yav[i] > 0 else 1.0

    def _get_minimum_tof(self):
        '''
        Converts the maximum energy transfer to neutron to an equivalent
        minimum tof. The distance from the sample to the detector is 2.4m (fixed) and
        source to sample is 0.695m. The result is the minimum tof from source to detector
        and the result is returned in microseconds.
        '''
        nom_velocity = 437.4 * math.sqrt(self._efixed)
        max_meV = self._efixed + self._max_energy_gain
        max_velocity = 437.4 * math.sqrt(max_meV)
        min_tof = 0.695 / nom_velocity + 2.4 / max_velocity
        return min_tof * 1e6

    def _adjust_frame_overlap(self, eventlist, gate_period, min_tof):

        tofs, pulsetimes = eventlist.getTofs(), eventlist.getPulseTimes()

        # shift the fast event to the end of the frame
        cnd = tofs < min_tof
        tofs[cnd] += gate_period

        # clear and read events
        eventlist.clear(False)
        for tof, pt in zip(tofs, pulsetimes):
            eventlist.addEventQuickly(tof, pt)

    def _load_and_reduce(self, output_ws, analyse_runs, convert_dE=True, energy_bins=None):

        # check if no runs or already loaded
        if not analyse_runs:
            return None
        if output_ws in self._intermediate_ws:
            return output_ws

        self._load_merge(analyse_runs, output_ws, self._analyse_load_opts)

        # if minimum_tof then shift the tof by the gate period
        if self._max_energy_gain > 0.0:
            ows = mtd[output_ws]
            try:
                gate_period = ows.getRun().getProperty('GatePeriod').value[0]
            except TypeError:
                gate_period = ows.getRun().getProperty('GatePeriod').value
            minimum_tof = self._get_minimum_tof()
            for i in range(ows.getNumberHistograms()):
                evl = ows.getSpectrum(i)
                self._adjust_frame_overlap(evl, gate_period, minimum_tof)

            # reset the X values
            maxTOF = ows.getTofMax()
            minTOF = ows.getTofMin()
            paramstr = '{}, {}, {}'.format(minTOF, maxTOF - minTOF, maxTOF)
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws,
                  Params=paramstr, PreserveEvents=True)

        if convert_dE:
            # the energy conversion for analysed data uses the existing
            # unit conversion
            ConvertUnits(InputWorkspace=output_ws, OutputWorkspace=output_ws,
                         Target='DeltaE', EMode='Direct',
                         EFixed=self._efixed, AlignBins=True)
            use_energy_bins = energy_bins if energy_bins else self._ev_range
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws,
                  Params=use_energy_bins, PreserveEvents=False)
        else:
            Rebin(InputWorkspace=output_ws, OutputWorkspace=output_ws,
                  Params='0, 1, 6000', PreserveEvents=True)

        self._intermediate_ws.append(output_ws)

        return output_ws

    def _build_temp_fpath(self, run, dataset, name):
        # returns basename_suffix.nxs as it will be saved as a nexus file
        # if the name includes dataset greater than 0 append it to the name
        dset = '_{}'.format(dataset) if dataset > 0 else ''
        basename = os.path.basename(run).split('.')[0]
        tmp = os.path.join(self._temp_folder, basename + dset + name + '.nxs')
        return os.path.normpath(tmp)

    def _restore_runs_from_scratch_folder(self, output_ws, runs, lopts):

        base_run, base_ix = split_run_index(runs[0])

        # look for a matching workspace that is a subset
        # of the runs required with the same load options
        # returning the merged workspace and the file that are already loaded
        loaded, empty_ws = [], ''
        fpath = self._build_temp_fpath(base_run, base_ix, output_ws)
        if not os.path.isfile(fpath):
            return loaded, empty_ws

        # load the file and check if the merged workspaces and loader options match
        LoadNexusProcessed(Filename=fpath, OutputWorkspace=output_ws)
        mrun = mtd[output_ws].getRun()
        run_lopts = mrun.getProperty('loader_options').value
        dump_lopts = json.dumps(lopts, sort_keys=True)

        if run_lopts == dump_lopts:
            # build the list of loaded files and test if it is subset of the
            # required runs
            for prop in mrun.getProperties():
                if re.match(r'^merged_[0-9]+$', prop.name):
                    loaded.append(prop.value)

            def base_index(run):
                base, ix = split_run_index(run)
                return os.path.basename(base) + ':{}'.format(ix)

            needed = [base_index(run) for run in runs]
            if set(loaded) <= set(needed):
                # if it is a subset it can continue by adding the
                # additional files
                return sorted(loaded), output_ws

        # if gets to here then match failed and it needs to reload
        # all the files as the events were all merged
        DeleteWorkspace(Workspace=output_ws)
        return [], empty_ws

    def _copy_to_scratch_folder(self, output_ws, loaded, lopts):

        # add the lopts to the properties
        run = mtd[output_ws].getRun()
        run.addProperty('loader_options', json.dumps(
            lopts, sort_keys=True), True)

        # add the list of merged files that make up the work space
        # to the properties
        for (ix, name) in enumerate(loaded):
            run.addProperty('merged_'+str(ix), name, True)

        fpath = self._build_temp_fpath(loaded[0], 0, output_ws)
        SaveNexusProcessed(InputWorkspace=output_ws, Filename=fpath)

    def _load_run_from_scratch(self, run, dataset, loader, lopts, output_ws):

        # looks for a nxs file file in the temp folder
        fpath = self._build_temp_fpath(run, dataset, '')
        load_ok = False
        if os.path.isfile(fpath):
            # load the file and confirm the tof correction is within 1usec
            LoadNexusProcessed(Filename=fpath, OutputWorkspace=output_ws)
            load_ok = True

            # check if tof calibration is enabled else the
            # tof correction agrees, otherwise delete and reload
            params = [('LambdaOnTwoMode', 'LambdaOnTwoMode', 0),
                      ('SelectDataset', 'SelectDataset', 0.1)]
            if lopts['CalibrateTOFBias']:
                params.append(('CalibrateTOFBias', 'CalibrateTOF', 0.1))
            else:
                params.append(('TimeOfFlightBias', 'TOFCorrection', 1.0))
            mrun = mtd[output_ws].getRun()

            for (otag, rtag, tol) in params:
                try:
                    set_pm = lopts[otag]
                    try:
                        act_pm = mrun.getProperty(rtag).value[0]
                    except TypeError:
                        act_pm = mrun.getProperty(rtag).value
                    if isinstance(set_pm, bool):
                        if (act_pm != 0) == set_pm:
                            continue
                    elif math.fabs(act_pm - set_pm) <= tol:
                        continue
                except:
                    raise RuntimeError('Cannot find property {}'.format(rtag))

                # it gets here because tolerance exceeded or missing values
                # either way delete and restart
                DeleteWorkspace(Workspace=output_ws)
                load_ok = False
                break

        # if the nexus is missing load from the run and save as Nexus
        if not load_ok:
            # reload the full file without any tube selection
            lopts['BinaryEventPath'] = find_event_path(
                run, self._event_search_path)
            loader(Filename=run, OutputWorkspace=output_ws, **lopts)
            SaveNexusProcessed(InputWorkspace=output_ws, Filename=fpath)

    def _filter_workspace(self, ws_tag, output_ws, valid_tubes, valid_pixels):

        # the only options are the tube numbers and the pixel range
        # build a condition test per pixel/histogram to decide if it is kept

        event_ws = mtd[ws_tag]
        nhist = event_ws.getNumberHistograms()
        include = np.zeros(nhist, dtype=bool)

        # mask to the selected tubes
        if valid_tubes is None:
            include = True
        else:
            detectors = self._detector_spectra / self._pixels_per_tube
            for tube in sorted(seq_to_list(valid_tubes)):
                if tube < detectors:
                    lo = tube * self._pixels_per_tube
                    hi = lo + self._pixels_per_tube
                else:
                    lo = self._detector_spectra + tube - detectors
                    hi = lo + 1
                include[lo:hi] = True

        # mask to the valid pixel range
        if valid_pixels is not None:
            det_ids = np.arange(nhist)
            lo_ids = det_ids % self._pixels_per_tube >= valid_pixels[0]
            hi_ids = det_ids % self._pixels_per_tube <= valid_pixels[1]
            lo_ids[self._detector_spectra:] = True
            hi_ids[self._detector_spectra:] = True
            include = include * lo_ids * hi_ids

        # scan over the spectrum
        for i in range(nhist):
            if not include[i]:
                evl = event_ws.getSpectrum(i)
                evl.clear(False)

        # mask the spectra, exlicitly convert numpy.int32 to int as MaskDetectors fails
        mask = np.invert(include)
        masked_spectra = [int(x) for x in np.arange(nhist)[mask]]
        MaskDetectors(Workspace=ws_tag, SpectraList=masked_spectra)

        if ws_tag != output_ws:
            RenameWorkspace(InputWorkspace=ws_tag, OutputWorkspace=output_ws)

    def _load_merge(self, runs, output_ws, load_opts):
        """
        Loads and sums the event data.
        If a temp folder is provided it looks for an existing file in folder to
        reload the file if it matches the runs and loader options.

        The loader looks for the completely loaded or a proper subset that
        matches the loader option conditions. When loading individual files
        it also looks for a match .nxs file that matches the doppler phase
        value and then filters the events to the load options.
        If a new file is loaded it is loaded with filtering, saved as a .nxs
        file to the scratch folder and filtered appropriately.

        If no scratch folder is available it loads the files with the load
        options.

        """
        if len(runs) == 0:
            return None

        # get the loader for the collection of files - only expect
        # *.hdf - LoadPLN.
        # need to trap the exception as old data does not have all the
        # hdf parameters in the loader check
        base, _ = split_run_index(runs[0])
        try:
            winning_loader = FileLoaderRegistry.Instance().chooseLoader(base)
            loader_name = winning_loader.name()
            loader_version = winning_loader.version()
        except RuntimeError:
            loader_name = 'LoadPLN'
            loader_version = 1

        lopts = self._get_loader_options(loader_name,
                                         loader_version, load_opts)
        if loader_name == 'LoadPLN':
            loader = LoadPLN
        else:
            raise RuntimeError('Cannot find suitable loader')

        # if using a temp folder look for a matching workspace that is a subset
        # of the runs required and same load options returning the merged workspace
        # and the file that are already loaded
        loaded = []
        merged = ''
        updated = False
        if self._temp_folder is not None:
            loaded, merged = self._restore_runs_from_scratch_folder(
                output_ws, runs, lopts)

        for (ix, esource) in enumerate(runs):

            # esource contains dataset as an suffix fpath:n
            source, ds_index = split_run_index(esource)

            # if the file has been loaded as part of the temp load
            # update progress and skip to next

            basename = os.path.basename(source) + ':{}'.format(ds_index)
            if basename in loaded:
                self._progress.report('Loaded ' + esource)
                continue

            # load the source file, set update true
            tmp_ws = '_src_' + str(ix)
            run_opts = lopts.copy()
            run_opts['SelectDataset'] = ds_index

            # load from temp folder if available otherwise load directly
            if self._temp_folder is not None:
                self._load_run_from_scratch(
                    source, ds_index, loader, run_opts, tmp_ws)
            else:
                run_opts['BinaryEventPath'] = find_event_path(
                    source, self._event_search_path)
                loader(Filename=source, OutputWorkspace=tmp_ws, **run_opts)
            self._progress.report('Loaded ' + esource)
            loaded.append(basename)
            updated = True

            if ix == 0:
                merged = tmp_ws
            else:
                # combined the events to the merged output and add the last filename to
                # the run log
                m_run = mtd[merged].getRun()
                m_run.addProperty('merged_' + str(ix), loaded[-1], True)
                tmp_merged = '__tmp_' + merged
                MergeRuns(InputWorkspaces=[
                          merged, tmp_ws], OutputWorkspace=tmp_merged)

                DeleteWorkspace(Workspace=merged)
                DeleteWorkspace(Workspace=tmp_ws)
                RenameWorkspace(InputWorkspace=tmp_merged,
                                OutputWorkspace=merged)

        # update the end time log information in the merged workspace
        # to match the total duration
        m_run = mtd[merged].getRun()
        start_time = m_run.startTime()
        duration = int(1e9 * m_run.getProperty('dur').value)
        end_time = DateAndTime(start_time.totalNanoseconds() + duration)
        m_run.setStartAndEndTime(start_time, end_time)

        # update the title to reflect the data loaded
        title = ','.join(loaded[:1] + extract_runs(loaded[1:], collapse=True))
        mtd[merged].setTitle(title)

        if merged != output_ws:
            RenameWorkspace(InputWorkspace=merged, OutputWorkspace=output_ws)

        if self._temp_folder is not None and updated:
            self._append_ini_params(output_ws)
            self._copy_to_scratch_folder(output_ws, loaded, lopts)

        # finally filter for the range f valid pixels and doppler window if needed
        self._filter_workspace(output_ws, output_ws, valid_tubes=self._analyse_tubes,
                               valid_pixels=self._pixel_range)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(PelicanReduction)
