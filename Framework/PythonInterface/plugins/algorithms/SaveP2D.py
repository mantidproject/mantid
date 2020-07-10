import numpy as np
import sys
import math
from scipy.special import lambertw
from mantid.kernel import *
from mantid.api import *


class SaveP2D(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'Tools//Algorithms'

    def summary(self):
        """
        summary of the algorithm
        :return:
        """
        return "The algorithm used to create a multidimensional '.p2d' file from a 2D workspace."

    def name(self):
        return "SaveP2d"

    def seeAlso(self):
        return ["PowderReduceP2D", "Bin2DPowderDiffraction"]

    def PyInit(self):
        # Input file
        self.declareProperty(WorkspaceProperty('Workspace',
                                               '',
                                               direction=Direction.Input),
                             doc='Workspace that should be used.')
        # Output File
        self.declareProperty(FileProperty('OutputFile',
                                          '',
                                          action=FileAction.Save,
                                          direction=Direction.Input),
                             doc='Output File for ".p2d" Data.')
        # Manipulating Data ranges
        self.declareProperty(
            'RemoveNaN',
            True,
            direction=Direction.Input,
            doc='Remove DataPoints with NaN as intensity value')
        self.declareProperty(
            'RemoveNegatives',
            True,
            direction=Direction.Input,
            doc='Remove data points with negative intensity values')
        self.declareProperty(
            'CutData',
            False,
            direction=Direction.Input,
            doc=
            'Use the following inputs to limit data in Theta, lambda, d and dp'
        )
        self.declareProperty('tthMin',
                             50,
                             direction=Direction.Input,
                             doc='Minimum for tth values')
        self.declareProperty('tthMax',
                             120,
                             direction=Direction.Input,
                             doc='Maximum  for tth values')
        self.declareProperty('lambdaMin',
                             0.3,
                             direction=Direction.Input,
                             doc='Minimum  for lambda values')
        self.declareProperty('lambdaMax',
                             1.1,
                             direction=Direction.Input,
                             doc='Maximum  for lambda values')
        self.declareProperty('dMin',
                             0.11,
                             direction=Direction.Input,
                             doc='Minimum  for d values')
        self.declareProperty('dMax',
                             1.37,
                             direction=Direction.Input,
                             doc='Maximum  for d values')
        self.declareProperty('dpMin',
                             0.48,
                             direction=Direction.Input,
                             doc='Minimum  for dp values')
        self.declareProperty('dpMax',
                             1.76,
                             direction=Direction.Input,
                             doc='Maximum  for dp values')


# Create output file

    def PyExec(self):

        def wo_real(x):
            return np.real(lambertw(np.exp(x), 0))

        # Workspace name
        Workspace = self.getPropertyValue('Workspace')

        

        # Create Output File
        OutFile = self.getPropertyValue('OutputFile') + '.p2d'
        # Load Workspace data
        Data = mtd[Workspace]
        # output style
        form = '{:12.7f}'
        lform = '{:12.7f}   {:12.7f}   {:12.7f}   {:12.7f}   {:12.7f}\n'

        of = open(OutFile, 'w')
        with open(OutFile, 'w') as of:
            print('Exporting: ' + OutFile + '\n')
            # Create File header with additional information
            of.write('#Title: ' + Data.getTitle() + "\n")
            of.write('#Inst: ' + Data.getInstrument().getName() + ".prm\n")
            binning = form.format(
                Data.getDimension(0).getBinWidth()) + ' ' + form.format(
                    Data.getDimension(1).getBinWidth()) + "\n"
            of.write('#Binning: ddperp' + binning)
            of.write('#Bank: 1\n')
            of.write('#2theta   lambda   d-value   dp-value   counts\n')
            # Get number of bins
            ndp = Data.getDimension(1).getNBins()
            # create progress reporter
            pr = Progress(None, start=0.0, end=1.0, nreports=ndp)
            last_dp = -1
            last_d = -1

            # iterate over all dPerpendicular cuts
            for cdp in range(ndp):
                dp = Data.getDimension(1).getX(cdp)
                # skip if no new dPerpendicular cut
                if (dp == last_dp):
                    continue
                # create value of bin center from left and right border
                dp_center = (dp + Data.getDimension(1).getX(cdp + 1)) / 2.
                last_dp = dp
                pr.report("Processing")
                sys.stdout.write("{:4.0f}%".format(cdp * 100. / ndp))
                # iterate over all dSpacing values for the selected dPerpendicular value
                for cd in range(Data.getDimension(0).getNBins()):
                    d = Data.dataX(cdp)[cd]
                    # skip if d is the same as before
                    if (d == last_d):
                        break
                    last_d = d
                    Y = Data.dataY(cdp)[cd]
                    # skip NaN values for intensity if option is activated
                    if self.getPropertyValue('RemoveNaN') == '1':
                        if math.isnan(Y):
                            continue
                    # skip intensities <= 0 if option is selected
                    if self.getPropertyValue('RemoveNegatives') == '1':
                        if Y <= 0:
                            continue
                    # calculate 2theta and lambda from d and dPerpendicular
                    dsq = d**2
                    lhkl = np.sqrt(4. * dsq - wo_real(4. * dsq - np.log(0.25 / dsq) - dp_center**2))
                    thkl = 2. * np.arcsin(lhkl / 2. / d) / np.pi * 180.
                    # skip Values outside of specified data ranges if the option is activated
                    if self.getPropertyValue('CutData') == '1':
                        if self.getPropertyValue(
                                'tthMin') < thkl < self.getPropertyValue(
                                    'tthMax'):
                            if self.getPropertyValue(
                                    'lambdaMin'
                            ) < lhkl < self.getProeprtyValue('lambdaMax'):
                                if self.getPropertyValue(
                                        'dMin') < d < self.getPropertyValue(
                                            'dMax'):
                                    if self.getPropertyValue(
                                            'dpMin'
                                    ) < dp < self.getProeprtyValue('dpMax'):
                                        # write data into outfile after checking all data ranges positive
                                        of.write(
                                            lform.format(
                                                thkl, lhkl, d, dp_center, Y))
                                    else:
                                        continue
                                else:
                                    continue
                            else:
                                continue
                        else:
                            continue
                    else:
                        # Write data into outfile
                        of.write(lform.format(thkl, lhkl, d, dp_center, Y))
            print('\n\nExported: ' + OutFile + '\n')


AlgorithmFactory.subscribe(SaveP2D)
