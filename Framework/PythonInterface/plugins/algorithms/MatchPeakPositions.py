from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import Direction
import numpy as np


class MatchPeakPositions(PythonAlgorithm):

    def category(self):
        return "Utility\\Workspaces"

    def summary(self):
        return 'Shifts each spectrum according to input options; \n' \
                'If given only, each single spectrum will be centered around 0 [Unit] \n' \
                'If in addition input workspace 2 is given and shift_option is False, ws1 will be \n' \
                'shifted to match the peak positions of input workpsace 2'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace','', direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Input workspace')

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace2','', direction=Direction.Input),
                             doc='Optional input workspace that modifies shifting the mandatory input workspace. \n'
                                'If in addition ws2 is given and shift_option is True, ws1 will be shifted by the \n'
                                'number of bins that is required for ws2 to be centered')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace','', direction=Direction.Output),
                             doc='Output workspace')

        self.declareProperty(name='Masking',
                             defaultValue=False,
                             doc='Whether to mask shifted bins')

        self.declareProperty(name='MatchFromInputWorkspace2',
                             defaultValue=False,
                             doc='Option to shift the input workspace by number of bins (input workspace 2 to center)')

        self.declareProperty(name='OutputBoundaryBins',
                             direction=Direction.Output,
                             doc='Returns valid bin range')

    def validateInputs(self):
        issues = dict()

        if self.getPropertyValue('InputWorkspace2') is None and self.getPropertyValue('MatchFromInputWorkspace2') is True:
            issues['InputWorkspace2'] = 'ShiftOption requires a second input workspace.'

    def setUp(self):
        self.input_ws = self.getPropertyValue('InputWorkspace')
        self.input_2_ws = self.getPropertyValue('InputWorkspace2')
        output_ws = self.getPropertyValue('OutputWorkspace')

        self.masking = self.getPropertyValue('Masking')
        self.shift_option = self.getPropertyValue('ShiftOption')  # get_peak_positions with shift_spectra to move to separate algorithm

    def mask_reduced_ws(red, xstart, xend):
        """
        Args:
            red:      reduced workspace
            x:        x-values of the workspace (energy transfer), optional
            xstart:   MaskBins between 0 and xstart
            xend:     MaskBins between xend and x[-1]

        """
        x = mtd[red].readX(0)

        if xstart > 0:
            logger.debug('Mask bins smaller than %d' % (xstart + 1))
            MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[0], XMax=x[xstart])
        if xend < len(x) - 1:
            logger.debug('Mask bins larger than %d' % (xend - 1))
            MaskBins(InputWorkspace=red, OutputWorkspace=red, XMin=x[xend], XMax=x[-1])

        logger.notice('Bins out of range [%f %f] [Unit of X-axis] are masked' % (x[xstart], x[xend - 1]))

    def get_peak_position(ws, i):
        """
        Gives bin of the peak of i-th spectrum in the ws
        @param ws        :: input workspace
        @param i         :: spectrum index of input workspace
        @return          :: bin number of the peak position
        """
        __temp = ExtractSingleSpectrum(InputWorkspace=ws, WorkspaceIndex=i)

        __fit_table = FindEPP(InputWorkspace=__temp)

        # Mid bin number
        mid_bin = int(__temp.blocksize() / 2)

        # Bin number, where Y has its maximum
        y_values = __temp.readY(0)

        # Delete unused single spectrum
        DeleteWorkspace(__temp)

        # Bin range: difference between mid bin and peak bin should be in this range
        tolerance = int(mid_bin / 2)

        # Peak bin (not in energy)
        peak_bin = __fit_table.row(0)["PeakCentre"]

        # Reliable check for peak bin
        fit_status = __fit_table.row(0)["FitStatus"]

        if peak_bin < 0 or peak_bin > len(y_values) or \
                (fit_status != 'success') or (abs(peak_bin - mid_bin) > tolerance):
            # Fit failed (too narrow peak) or outside bin range
            if abs(np.argmax(y_values) - mid_bin) < tolerance:
                # Take bin of maximum peak
                peak_bin = np.argmax(y_values)
            else:
                # Take the center (i.e. do no shift the spectrum)
                peak_bin = mid_bin

        # Delete unused TableWorkspaces
        try:
            DeleteWorkspace('EPPfit_NormalisedCovarianceMatrix')
            DeleteWorkspace('EPPfit_Parameters')
            DeleteWorkspace(__fit_table)
        except ValueError:
            logger.debug('No Fit table available for deletion')

        return peak_bin

    def PyExec(self):

        """
        If only ws1 is given, each single spectrum will be centered
        If in addition ws2 is given and shift_option is False, ws1 will be shifted to match the peak positions of ws2
        If in addition ws2 is given and shift_option is True, ws1 will be shifted by the
        number of bins that is required for ws2 to be centered
        @param ws1                         ::   input workspace that will be shifted
        @param ws2                         ::   optional workspace according to which ws1 will be shifted
        @param shift_option                ::   option to shift ws1 by number of bins (ws2 to center)
        @return                            ::   bin numbers for masking
        """
        number_spectra = mtd[ws1].getNumberHistograms()
        size = mtd[ws1].blocksize()

        if ws2 is not None and \
                (size != mtd[ws2].blocksize() or number_spectra != mtd[ws2].getNumberHistograms()):
            logger.warning('Input workspaces should have the same number of bins and spectra')

        mid_bin = int(size / 2)

        # Initial values for bin range of output workspace. Bins outside this range will be masked
        start_bin = 0
        end_bin = size

        # Shift each single spectrum of the input workspace ws1
        for i in range(number_spectra):

            # Find peak positions in ws1
            logger.debug('Get peak position of spectrum %d' % i)
            peak_bin1 = get_peak_position(ws1, i)

            # If only one workspace is given as an input, this workspace will be shifted
            if ws2 is None:
                to_shift = peak_bin1 - mid_bin
            else:
                # Find peak positions in ws2
                peak_bin2 = get_peak_position(ws2, i)

                if not shift_option:
                    # ws1 will be shifted according to peak position of ws2
                    to_shift = peak_bin1 - peak_bin2
                else:
                    # ws1 will be shifted according to centered peak of ws2
                    to_shift = peak_bin2 - mid_bin

            # Shift Y and E values of spectrum i by a number of to_shift bins
            # Note the - sign, since np.roll shifts right if the argument is positive
            # while here if to_shift is positive, it means we need to shift to the left
            mtd[ws1].setY(i, np.roll(mtd[ws1].dataY(i), int(-to_shift)))
            mtd[ws1].setE(i, np.roll(mtd[ws1].dataE(i), int(-to_shift)))

            if (size - to_shift) < end_bin:
                end_bin = size - to_shift
                logger.debug('New right boundary for masking due to left shift by %d bins' % to_shift)
            elif abs(to_shift) > start_bin:
                start_bin = abs(to_shift)
                logger.debug('New left boundary for masking due to right shift by %d bins' % abs(to_shift))
            else:
                logger.debug('Shifting does not result in a new range for masking')

        # Mask bins to the left of the final bin range
        if masking is True:
            mask_reduced_ws(ws1, start_bin, end_bin)

        return

AlgorithmFactory.subscribe(MatchPeakPositions)
