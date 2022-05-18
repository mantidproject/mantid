# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import h5py

from mantid import config
from mantid.kernel import (Direction, StringArrayMandatoryValidator, StringArrayProperty, CompositeValidator)
from mantid.api import (PythonAlgorithm, FileProperty, FileAction, Progress)
from mantid.simpleapi import *  # noqa


def seq_to_list(iseqn):
    # convert a comma separated range of numbers returned as a list
    # first clean all whitespaces
    seqn = iseqn.replace(' ', '')
    nlist = []
    sqlist = seqn.split(',')
    for rg in sqlist:
        if rg == '':
            continue
        ss = rg.split('-')
        try:
            lo = int(ss[0])
            hi = int(ss[-1])
            if len(ss) == 1:
                nlist.append(lo)
            else:
                for i in range(lo, hi+1):
                    nlist.append(i)
        except ValueError:
            raise RuntimeError('Unexpected run sequence: {}'.format(seqn))

    return nlist


def cycle_and_runs(run_seq):
    ss = run_seq.split('::')
    if len(ss) == 2:
        return int(ss[0]), seq_to_list(ss[1])
    else:
        return None, seq_to_list(ss[0])


def expand_as_cycle_runs(sample_runs):
    cycle_runs = []
    for run_seq in sample_runs.split(';'):
        cycle, runs = cycle_and_runs(run_seq)
        for run in runs:
            cycle_runs.append((cycle, run))
    return cycle_runs


def copy_datset_nodes(src_hdf, dst_hdf, tags):
    with h5py.File(dst_hdf, 'r+') as f_dst:
        with h5py.File(src_hdf, 'r') as f_src:
            for tag in tags:
                sval = f_src[tag][()]
                dset = f_dst[tag]
                dset[...] = sval


class PelicanCrystalProcessing(PythonAlgorithm):

    def category(self):
        return "Workflow"

    def summary(self):
        return 'Performs crystal processing for ANSTO Pelican data.'

    def seeAlso(self):
        return ["NA"]

    def name(self):
        return "PelicanCrystalProcessing"

    def PyInit(self):

        mandatoryInputRuns = CompositeValidator()
        mandatoryInputRuns.add(StringArrayMandatoryValidator())
        self.declareProperty(StringArrayProperty('SampleRuns',
                                                 values=[],
                                                 validator=mandatoryInputRuns),
                             doc='Comma separated range of sample runs,\n'
                                 ' eg [cycle::] 7333-7341,7345')

        self.declareProperty(name='EmptyRuns',
                             defaultValue='',
                             doc='Optional path followed by comma separated range of runs,\n'
                                 'looking for runs in the sample folder if path not included,\n'
                                 '  eg [cycle::] 6300-6308')

        self.declareProperty(name='ScaleEmptyRuns',
                             defaultValue=1.0,
                             doc='Scale the empty runs prior to subtraction')

        self.declareProperty(name='CalibrationRuns',
                             defaultValue='',
                             doc='Optional path followed by comma separated range of runs,\n'
                                 'looking for runs in the sample folder if path not included,\n'
                                 '  eg [cycle::] 6350-6365')

        self.declareProperty(name='EmptyCalibrationRuns',
                             defaultValue='',
                             doc='Optional path followed by comma separated range of runs,\n'
                                 'looking for runs in the sample folder if path not included,\n'
                                 '  eg [cycle::] 6370-6375')

        self.declareProperty(name='EnergyTransfer',
                             defaultValue='0.0, 0.02, 3.0',
                             doc='Energy transfer range in meV expressed as min, step, max')

        self.declareProperty(name='MomentumTransfer',
                             defaultValue='',
                             doc='Momentum transfer range in inverse Angstroms,\n'
                                 'expressed as min, step, max. Default estimates\n'
                                 'the max range based on energy transfer.')

        self.declareProperty(name='LambdaOnTwoMode', defaultValue=False,
                             doc='Set if instrument running in lambda on two mode.')

        self.declareProperty(name='FrameOverlap', defaultValue=False,
                             doc='Set if the energy transfer extends over a frame.')

        self.declareProperty(name='FixedDetector', defaultValue=True,
                             doc='Fix detector positions to the first run')

        self.declareProperty(FileProperty('ScratchFolder', '',
                                          action=FileAction.OptionalDirectory,
                                          direction=Direction.Input),
                             doc='Path to save and restore merged workspaces.')

        mandatoryOutputFolder = CompositeValidator()
        mandatoryOutputFolder.add(StringArrayMandatoryValidator())
        self.declareProperty(FileProperty('OutputFolder', '',
                                          action=FileAction.Directory,
                                          direction=Direction.Input),
                             doc='Path to save the output nxspe files.')

        self.declareProperty(FileProperty('ConfigurationFile', '',
                                          action=FileAction.OptionalLoad,
                                          extensions=['ini']),
                             doc='Optional: INI file to override default processing values.')

        self.declareProperty(name='KeepReducedWorkspace', defaultValue=False,
                             doc='Keep the last reduced workspace.')

    def PyExec(self):

        self._first_run = None

        # Get the list of data files from the runs
        sample_runs = self.getPropertyValue('SampleRuns')
        output_folder = self.getPropertyValue('OutputFolder')
        scratch_folder = self.getPropertyValue('ScratchFolder')
        keep_reduced_ws = self.getProperty('KeepReducedWorkspace').value
        cycle_runs = expand_as_cycle_runs(sample_runs)

        # set up progress bar
        steps = len(cycle_runs) + 1
        self._progress = Progress(
            self, start=0.0, end=1.0, nreports=steps)
        #grouped = []

        saveFolder = scratch_folder if scratch_folder else config['defaultsave.directory']
        for cycle, run in cycle_runs:

            if cycle:
                srun = str(cycle) + ':: ' + str(run)
            else:
                srun = str(run)
            self._progress.report("Processing run {}, ".format(run))

            PelicanReduction(SampleRuns=srun, EmptyRuns=self.getPropertyValue('EmptyRuns'),
                             EnergyTransfer=self.getPropertyValue(
                                      'EnergyTransfer'),
                             MomentumTransfer=self.getPropertyValue('MomentumTransfer'), Processing='NXSPE',
                             LambdaOnTwoMode=self.getProperty(
                                      'LambdaOnTwoMode').value,
                             FrameOverlap=self.getProperty(
                                      'FrameOverlap').value,
                             ScratchFolder=scratch_folder, OutputWorkspace='nxspe',
                             ConfigurationFile=self.getPropertyValue('ConfigurationFile'))

            # the nxspe file named 'nxspe_spe_2D.nxspe'is moved to the output folder and renamed
            dfile = 'run_{:d}.nxspe'.format(run)
            dpath = os.path.join(output_folder, dfile)
            if os.path.isfile(dpath):
                os.remove(dpath)
            spath = os.path.join(saveFolder, 'nxspe_spe_2D.nxspe')
            os.rename(spath, dpath)

            if not self._first_run:
                self._first_run = dpath
            elif self.getProperty('FixedDetector').value:
                copy_datset_nodes(self._first_run, dpath, ['/nxspe_spe_2Ddet/data/azimuthal',
                                                           '/nxspe_spe_2Ddet/data/azimuthal_width',
                                                           '/nxspe_spe_2Ddet/data/polar',
                                                           '/nxspe_spe_2Ddet/data/polar_width'])

            # the pelican reduction saves a tempory file 'PLN00nnnn_sample.nxs' and 'PLN00nnnn.nxs' to
            # speed up processing, remove these files to save space
            if scratch_folder:
                for sfile in ['PLN{:07d}_sample.nxs', 'PLN{:07d}.nxs']:
                    tfile = sfile.format(run)
                    tpath = os.path.join(scratch_folder, tfile)
                    os.remove(tpath)

        # delete the workspace, as it is only temporary
        if not keep_reduced_ws:
            self._progress.report("Cleaning up file,")
            DeleteWorkspace(Workspace='nxspe_spe_2D')


# Register algorithm with Mantid
AlgorithmFactory.subscribe(PelicanCrystalProcessing)
