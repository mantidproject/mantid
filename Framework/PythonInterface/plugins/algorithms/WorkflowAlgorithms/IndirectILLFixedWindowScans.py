#pylint: disable=no-init
from __future__ import (absolute_import, division, print_function)


from mantid.simpleapi import *
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, PropertyMode, MatrixWorkspaceProperty, \
                       WorkspaceGroupProperty, FileProperty, MultipleFileProperty, FileAction, NumericAxis, \
                       WorkspaceGroup, Progress
from mantid.kernel import StringListValidator, StringMandatoryValidator, IntBoundedValidator, \
                          FloatBoundedValidator, Direction, config, logger
import math, os.path, numpy as np
import IndirectILLReduction


# possibility to replace by the use of SelectNexusFilesByMetadata
def select_elastic(workspace):
    """
    Select fixed-window scan
    @param workspace :: input ws name
    @return   ::
    """
    run_object = mtd[workspace].getRun()
    run_number = mtd[workspace].getRunNumber()

    velocity_profile = 1
    energy = -1
    frequency = -1

    inelastic = False

    if run_object.hasProperty('Doppler.velocity_profile'):
        velocity_profile = run_object.getLogData('Doppler.velocity_profile').value
    else:
        logger.error('Run {0} has no Doppler.velocity_profile'.format(run_number))

    if run_object.hasProperty('Doppler.frequency'):
        frequency = run_object.getLogData('Doppler.frequency').value
    else:
        logger.error('Run {0} has no Doppler.frequency'.format(run_number))

    if run_object.hasProperty('Doppler.maximum_delta_energy'):
        energy = run_object.getLogData('Doppler.maximum_delta_energy').value
    elif run_object.hasProperty('Doppler.delta_energy'):
        energy = run_object.getLogData('Doppler.delta_energy').value
    else:
        logger.error('Run {0} has neither Doppler.maximum_delta_energy nor Doppler.delta_energy'.
                     format(run_number))

    if (energy > 0.0 or frequency > 0.0):
        inelastic = True
        logger.information('Run {0} elastic FWS data'.format(run_number))
    else:
        logger.information('Run {0} not elastic FWS data'.format(run_number))

    return inelastic


def select_elastic(workspace):
    """
    Select fixed-window scan
    @param workspace :: input ws name
    @return   ::
    """
    run_object = mtd[workspace].getRun()
    run_number = mtd[workspace].getRunNumber()

    velocity_profile = 1
    energy = -1
    frequency = -1

    elastic = False

    # Doppler.speed

    if run_object.hasProperty('Doppler.velocity_profile'):
        velocity_profile = run_object.getLogData('Doppler.velocity_profile').value
    else:
        logger.error('Run {0} has no Doppler.velocity_profile'.format(run_number))

    if run_object.hasProperty('Doppler.frequency'):
        frequency = run_object.getLogData('Doppler.frequency').value
    else:
        logger.error('Run {0} has no Doppler.frequency'.format(run_number))

    if run_object.hasProperty('Doppler.maximum_delta_energy'):
        energy = run_object.getLogData('Doppler.maximum_delta_energy').value
    elif run_object.hasProperty('Doppler.delta_energy'):
        energy = run_object.getLogData('Doppler.delta_energy').value
    else:
        logger.error('Run {0} has neither Doppler.maximum_delta_energy nor Doppler.delta_energy'.
                     format(run_number))

    if (energy == 0.0 or frequency == 0.0):
        elastic = True
        logger.information('Run {0} inelastic FWS data'.format(run_number))
    else:
        logger.information('Run {0} not inelastic FWS data'.format(run_number))

    return elastic


class IndirectILLFixedWindowScans(DataProcessorAlgorithm):

    _run_file = None
    _fws_type = None
    _out_ws = None
    _map_file = None
    _instrument = None

    def category(self):
        return 'Workflow\\Reduction'

    def summary(self):
        return 'Reduction for IN16B elastic and inelastic fixed-window scans.'

    def name(self):
        return "IndirectILLFixedWindowScans"

    def PyInit(self):
        self.declareProperty(MultipleFileProperty(name='Run',extensions=['nxs']),
                             doc='List of input file (s)')

        self.declareProperty(name='Elastic',
                             defaultValue=True,
                             doc='Select fixed-window scan FWS data: elastic')

        self.declareProperty(name='Inelastic',
                             defaultValue=False,
                             doc='Select fixed-window scan FWS data: inelastic')

        self.declareProperty(WorkspaceGroupProperty('OutputWorkspace', 'output',
                                                    direction=Direction.Output),
                             doc='Output workspace group')

    def validateInputs(self):

        issues = dict()
        if self.getProperty('Elastic').value and self.getProperty('Inelastic').value:
            issues['Elastic'] = 'Select elastic or inelastic scan'

        return issues

    def setUp(self):

        self._run_file = self.getPropertyValue('Run')
        self._fws_elastic = self.getProperty('Elastic').value
        self._fws_inelastic = self.getProperty('Inelastic').value
        self._out_ws = self.getPropertyValue('OutputWorkspace')

    def PyExec(self):
        self.setUp()

        self.log().information('Call IndirectILLReduction for .nxs file(s) : %s' % self._run_file)
        # Set explicitely QENS to False (to avoid?)
        IndirectILLReduction(Run=self._run_file, QENS=False, FWS=True, SumRuns=False, UnmirrorOption=1,
                             OutputWorkspace=self._out_ws)

        not_selected_runs = []

        # check if it is a workspace or workspace group and perform reduction correspondingly
        if isinstance(mtd[self._out_ws], WorkspaceGroup):

            # figure out number of progress reports, i.e. one for each input workspace/file
            progress = Progress(self, start=0.0, end=1.0, nreports=mtd[self._out_ws].size())

            # traverse over items in workspace group and reduce individually
            for i in range(mtd[self._out_ws].size()):
                # get the run number (if it is summed the run number of the first ws will be for the sum)
                run = format(mtd[self._out_ws].getItem(i).getName())

                progress.report("Reducing run #" + run)

                if (self._fws_elastic and select_elastic(run)) or (self._fws_inelastic and select_inelastic(run)):
                    self._reduce_run(run)
                else:
                    not_selected_runs.append(run)
        else:

            run = format(mtd[self._out_ws].getName())

            if (self._fws_elastic and select_elastic(run)) or (self._fws_inelastic and select_inelastic(run)):
                self._reduce_run(run)
            else:
                not_selected_runs.append(run)

        # remove any loaded non-QENS type data if was given:
        for not_selected_ws in not_selected_runs:
            DeleteWorkspace(not_selected_ws)

        # Throw meaningful error when all input workspaces are deleted
        try:
            if mtd[self._out_ws].size() > 0:
                self.setProperty('OutputWorkspace', self._out_ws)
        except KeyError:
            raise ValueError('No valid output workspace(s), check data type')

    def _reduce_run(self, run):
        """
        Performs the reduction for a given single run
        @param run :: string of run number to reduce, will be overridden
        """
        self.log().information('Reducing run #' + run)

        x = mtd[run].readX(0)
        y = mtd[run].readY(0)
        peak_pos = np.nanargmax(y)

        if self._fws_elastic:
            x_min = x[peak_pos] - 2
            x_max = x[peak_pos] + 2
            logger.information('EFWS scan from {0} to {1}'.format(x_min, x_max))
            Integration(InputWorkspace=run, OutputWorkspace=run,
                        RangeLower=x_min, RangeUpper=x_max)

        elif self._fws_inelastic:
            x_min = float(0)
            x_max = float(3)
            logger.information('IFWS scan outside range [{0} {1}]'.format(x_min, x_max))
            Integration(InputWorkspace=run, OutputWorkspace='__left',
                        RangeLower=x_min, RangeUpper=x_max)
            x_min = float(len(x) - 3)
            x_max = float(len(x))
            Integration(InputWorkspace=run, OutputWorkspace='__right',
                        RangeLower=x_min, RangeUpper=x_max)
            Plus(LHSWorkspace='__left', RHSWorkspace='__right',
                 OutputWorkspace=run)
            DeleteWorkspace('__left')
            DeleteWorkspace('__right')

AlgorithmFactory.subscribe(IndirectILLFixedWindowScans)
