# pylint: disable=too-many-branches
from __future__ import (absolute_import, division, print_function)
from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import Direction
import numpy as np


def mask_reduced_ws(ws_to_mask, xstart, xend):
    """
    Calls MaskBins twice, for masking the first and last bins of a workspace
    Args:
        red:      reduced workspace
        xstart:   MaskBins between x[0] and x[xstart]
        xend:     MaskBins between x[xend] and x[-1]

    """
    x_values = ws_to_mask.readX(0)

    if xstart > 0:
        logger.debug('Mask bins smaller than {0}'.format(xstart))
        MaskBins(InputWorkspace=ws_to_mask, OutputWorkspace=ws_to_mask, XMin=x_values[0], XMax=x_values[xstart])
    else:
        logger.debug('No masking due to x bin < 0!: {0}'.format(xstart))
    if xend < len(x_values) - 1:
        logger.debug('Mask bins larger than {0}'.format(xend))
        MaskBins(InputWorkspace=ws_to_mask, OutputWorkspace=ws_to_mask, XMin=x_values[xend + 1], XMax=x_values[-1])
    else:
        logger.debug('No masking due to x bin >= len(x_values) - 1!: {0}'.format(xend))

    if xstart > 0 and xend < len(x_values) - 1:
        logger.notice('Bins out of range {0} {1} [Unit of X-axis] are masked'.format(x_values[xstart],
                                                                                     x_values[xend + 1]))


class MatchPeaks(PythonAlgorithm):
    def __init__(self):

        PythonAlgorithm.__init__(self)

        # Mandatory workspaces
        self._input_ws = None

        # Optional workspace / tables
        self._input_2_ws = None
        self._input_table = None
        self._output_bin_range = None

        # Bool flags
        self._masking = None
        self._match_option = None

    def category(self):
        return "Transforms"

    def summary(self):
        return 'Shift bins of each spectrum of the input workspace such that peak positions match '\
               '(i.e. the center bin, the corresponding peak positions of a second input workspace)'

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Input),
                             doc='Input workspace')

        self.declareProperty(MatrixWorkspaceProperty('InputWorkspace2',
                                                     defaultValue='',
                                                     direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             doc='Input workspace for extracting its peak positions')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace',
                                                     defaultValue='',
                                                     direction=Direction.Output),
                             doc='Shifted output workspace')

        self.declareProperty('MaskBins',
                             defaultValue=False,
                             doc='Whether to mask shifted bins')

        self.declareProperty('MatchInput2ToCenter',
                             defaultValue=False,
                             doc='Match peak bins such that InputWorkspace2 would be centered')

        self.declareProperty(ITableWorkspaceProperty('BinRangeTable',
                                                     defaultValue='',
                                                     direction=Direction.Output,
                                                     optional=PropertyMode.Optional),
                             doc='Table workspace that contains a bin range for masking')

    def setUp(self):
        self._input_ws = self.getPropertyValue('InputWorkspace')
        self._input_2_ws = self.getPropertyValue('InputWorkspace2')
        self._output_ws = self.getPropertyValue('OutputWorkspace')
        self._masking = self.getProperty('MaskBins').value
        self._match_option = self.getProperty('MatchInput2ToCenter').value
        self._output_bin_range = self.getPropertyValue('BinRangeTable')

    def validateInputs(self):
        issues = dict()
        input1 = self.getPropertyValue('InputWorkspace')
        input2 = self.getPropertyValue('InputWorkspace2')
        if input2:
            if mtd[input1].blocksize() != mtd[input2].blocksize():
                issues['InputWorkspace2'] = 'Incompatible same number of bins'
            if mtd[input1].getNumberHistograms() != mtd[input2].getNumberHistograms():
                issues['InputWorkspace2'] = 'Incompatible number of spectra'
            if np.all(mtd[input1].extractX() - mtd[input2].extractX()):
                issues['InputWorkspace2'] = 'Incompatible x-values'

            if np.any(np.isnan(mtd[input2].extractY())):
                issues['InputWorkspace2'] = 'Y-values contain nans'
            if np.any(np.isnan(mtd[input2].extractE())):
                issues['InputWorkspace2'] = 'E-values contain nans'
            if np.any(np.isinf(mtd[input2].extractY())):
                issues['InputWorkspace2'] = 'Y-values contain infs'
            if np.any(np.isinf(mtd[input2].extractE())):
                issues['InputWorkspace2'] = 'E-values contain infs'
        if input1:
            if np.any(np.isnan(mtd[input1].extractY())):
                issues['InputWorkspace'] = 'Y-values contain nans'
            if np.any(np.isnan(mtd[input1].extractY())):
                issues['InputWorkspace'] = 'E-values contain nans'
            if np.any(np.isinf(mtd[input1].extractY())):
                issues['InputWorkspace'] = 'Y-values contain infs'
            if np.any(np.isinf(mtd[input1].extractY())):
                issues['InputWorkspace'] = 'E-values contain infs'

        return issues

    def PyExec(self):
        self.setUp()

        output_ws = CloneWorkspace(InputWorkspace=mtd[self._input_ws], OutputWorkspace=self._output_ws)

        size = mtd[self._input_ws].blocksize()

        mid_bin = int(size / 2)

        # Find peak positions in input workspace
        peak_bins1 = self._get_peak_position(output_ws)
        # Find peak positions in second input workspace
        if self._input_2_ws:
            peak_bins2 = self._get_peak_position(mtd[self._input_2_ws])
        elif self._input_table:
            peak_bins2 = self._get_peak_position(mtd[self._input_table])

        self.log().notice('Peak bins {0}: {1}'.format(self._input_ws, peak_bins1))
        if self._input_2_ws or self._input_table:
            self.log().notice('Peak bins {0} {1}: {2}'.format(self._input_2_ws, self._input_table, peak_bins2))

        # All bins must be positive and larger than zero
        if not self._input_2_ws and not self._input_table:
            # Input workspace will match center
            to_shift = peak_bins1 - mid_bin * np.ones(mtd[self._input_ws].getNumberHistograms())
        elif self._match_option:
            # Input workspace will be shifted according to centered peak of second input workspace
            to_shift = peak_bins2 - mid_bin * np.ones(mtd[self._input_ws].getNumberHistograms())
        else:
            # Input workspace will be shifted according to peak position of second input workspace
            to_shift = peak_bins1 - peak_bins2

        for i in range(output_ws.getNumberHistograms()):
            # Shift Y and E values of spectrum i
            output_ws.setY(i, np.roll(output_ws.readY(i), int(-to_shift[i])))
            output_ws.setE(i, np.roll(output_ws.readE(i), int(-to_shift[i])))
        self.log().debug('Shift array: {0}'.format(to_shift))

        # Final treatment of bins (masking, produce output)
        min_bin = 0
        max_bin = size

        if np.min(to_shift) < 0:
            max_bin = size - 1 - abs(np.min(to_shift))

        if np.max(to_shift) > 0:
            min_bin = np.max(to_shift)

        if self._masking:
            mask_reduced_ws(output_ws, min_bin, max_bin)

        if self._output_bin_range != '':
            # Create table with its columns containing bin range
            bin_range = CreateEmptyTableWorkspace(OutputWorkspace=self._output_bin_range)
            bin_range.addColumn(type="double", name='MinBin')
            bin_range.addColumn(type="double", name='MaxBin')

            bin_range.addRow({'MinBin': min_bin, 'MaxBin': max_bin})
            self.setProperty('BinRangeTable', bin_range)

        # Set output properties
        self.setProperty('OutputWorkspace', output_ws)

        DeleteWorkspace(output_ws)

        return

    def _get_peak_position(self, input_ws):
        """
        Gives bin of the peak of each spectrum in the input_ws
        @param input_ws  :: input workspace, can be a table workspace
        @return          :: bin numbers of the peak positions
        """

        if isinstance(input_ws, MatrixWorkspace):
            fit_table = FindEPP(InputWorkspace=input_ws)
        elif isinstance(input_ws, ITableWorkspace):
            fit_table = input_ws
        else:
            logger.error('Workspace not defined')

        # Mid bin number
        mid_bin = int(mtd[self._input_ws].blocksize() / 2)

        # Initialisation
        peak_bin = np.ones(mtd[self._input_ws].getNumberHistograms()) * mid_bin
        peak_plus_error = np.zeros(mtd[self._input_ws].getNumberHistograms())

        # Bin range: difference between mid bin and peak bin should be in this range
        tolerance = int(mid_bin / 2)

        x_values = mtd[self._input_ws].readX(0)

        for i in range(mtd[self._input_ws].getNumberHistograms()):
            fit = fit_table.row(i)

            # Bin number, where Y has its maximum
            y_values = mtd[self._input_ws].readY(i)

            peak_pos_error = fit["PeakCentreError"] + fit["PeakCentre"]

            if peak_pos_error >= x_values[0]:
                if peak_pos_error <= x_values[-1]:
                    if fit["FitStatus"] == 'success':
                        peak_plus_error[i] = mtd[self._input_ws].binIndexOf(peak_pos_error)
                        if abs(peak_plus_error[i] - mid_bin) < tolerance:
                            peak_bin[i] = mtd[self._input_ws].binIndexOf(fit["PeakCentre"])
                            logger.debug('Fit success, peak inside tolerance')
                        else:
                            logger.debug('Peak outside tolerance, do not shift spectrum')
                    elif abs(np.argmax(y_values) - mid_bin) < tolerance:
                        peak_bin[i] = np.argmax(y_values)
                        logger.debug('Take maximum peak position {0}'.format(peak_bin[i]))
                    else:
                        logger.debug('Fit failed and peak outside tolerance, do not shift spectrum')
                else:
                    logger.debug('Peak x-value {0} > x-end {1}, do not shift spectrum'.format(peak_pos_error,
                                                                                            x_values[-1]))
            else:
                logger.debug('Peak x-value {0} < x-begin {1}, do not shift spectrum'.format(peak_pos_error,
                                                                                            x_values[0]))

        # Delete unused TableWorkspaces
        try:
            DeleteWorkspace('EPPfit_Parameters')
        except ValueError:
            logger.debug('No EPPfit_Parameters table available for deletion')

        try:
            DeleteWorkspace('EPPfit_NormalisedCovarianceMatrix')
        except ValueError:
            logger.debug('No EPPfit_NormalisedCovarianceMatrix table available for deletion')

        DeleteWorkspace(fit_table)

        return peak_bin


AlgorithmFactory.subscribe(MatchPeaks)
