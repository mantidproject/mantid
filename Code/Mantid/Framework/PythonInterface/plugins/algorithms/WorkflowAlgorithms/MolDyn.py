#pylint: disable=invalid-name,no-init
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

import os


#pylint: disable=too-many-instance-attributes
class MolDyn(PythonAlgorithm):

    _plot = None
    _save = None
    _sam_path = None
    _symmetrise = None
    _functions = None
    _emax = None
    _res_ws = None
    _out_ws = None
    _mtd_plot = None


    def category(self):
        return 'Workflow\\Inelastic;PythonAlgorithms;Inelastic;Simulation'


    def summary(self):
        return 'Imports and processes simulated functions from nMOLDYN.'


    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '',
                                          action=FileAction.Load,
                                          extensions=['.cdl', '.dat']),
                                          doc='File path for data')

        self.declareProperty(StringArrayProperty('Functions'),
                             doc='The Function to use')

        self.declareProperty(WorkspaceProperty('Resolution', '', Direction.Input, PropertyMode.Optional),
                             doc='Resolution workspace')

        self.declareProperty(name='MaxEnergy', defaultValue='',
                             doc='Crop the result spectra at a given energy (leave blank for no crop)')

        self.declareProperty(name='SymmetriseEnergy', defaultValue=False,
                             doc='Symmetrise functions in energy about x=0')

        self.declareProperty(name='Plot', defaultValue='None',
                             validator=StringListValidator(['None', 'Spectra', 'Contour', 'Both']),
                             doc='Plot result workspace')

        self.declareProperty(name='Save', defaultValue=False,
                             doc='Save result workspace to nexus file in the default save directory')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '', Direction.Output),
                             doc='Output workspace name')


    def validateInputs(self):
        issues = dict()

        symm = self.getProperty('SymmetriseEnergy').value
        res_ws = self.getPropertyValue('Resolution')
        e_max = self.getPropertyValue('MaxEnergy')

        if res_ws is not '' and e_max is '':
            issues['MaxEnergy'] = 'MaxEnergy must be set when convolving with an instrument resolution'

        if res_ws is not '' and not symm:
            issues['SymmetriseEnergy'] = 'Must symmetrise energy when convolving with instrument resolution'

        return issues


    #pylint: disable=too-many-branches
    def PyExec(self):
        from IndirectImport import import_mantidplot
        self._mtd_plot = import_mantidplot()

        # Do setup
        self._setup()

        # Run nMOLDYN import
        LoadNMoldyn3Ascii(Filename=self.getPropertyValue('Filename'),
                          OutputWorkspace=self._out_ws,
                          Functions=self.getPropertyValue('Functions'))

        # Do processing specific to workspaces in energy
        if isinstance(mtd[self._out_ws], WorkspaceGroup):
            for ws_name in mtd[self._out_ws].getNames():
                if mtd[ws_name].getAxis(0).getUnit().unitID() == 'Energy':
                    # Get an XMax value, default to max energy if not cropping
                    e_max = mtd[ws_name].dataX(0).max()
                    logger.debug('Max energy in workspace %s: %f' % (ws_name, e_max))

                    if self._emax is not None:
                        if self._emax > e_max:
                            raise ValueError('MaxEnergy crop is out of energy range for function %s' % ws_name)
                        e_max = self._emax

                    # If we are going to Symmetrise then there is no need to crop
                    # as the Symmetrise algorithm will do this
                    if self._symmetrise:
                        # Symmetrise the sample workspace in x=0
                        Symmetrise(InputWorkspace=ws_name,
                                   XMin=0,
                                   XMax=e_max,
                                   OutputWorkspace=ws_name)

                    elif self._emax is not None:
                        CropWorkspace(InputWorkspace=ws_name,
                                      OutputWorkspace=ws_name,
                                      XMax=self._emax)

        # Do convolution if given a resolution workspace
        if self._res_ws is not '':
            # Create a workspace with enough spectra for convolution
            num_sample_hist = mtd[self._out_ws].getItem(0).getNumberHistograms()
            resolution_ws = self._create_res_ws(num_sample_hist)

            # Convolve all workspaces in output group
            for ws_name in mtd[self._out_ws].getNames():
                if ws_name.lower().find('sqw') != -1:
                    self._convolve_with_res(resolution_ws, ws_name)
                else:
                    logger.information('Ignoring workspace %s in convolution step' % ws_name)

            # Remove the generated resolution workspace
            DeleteWorkspace(resolution_ws)

        # Save result workspace group
        if self._save:
            workdir = config['defaultsave.directory']
            out_filename = os.path.join(workdir, self._out_ws + '.nxs')
            logger.information('Creating file: %s' % out_filename)
            SaveNexus(InputWorkspace=self._out_ws, Filename=out_filename)

        # Set the output workspace
        self.setProperty('OutputWorkspace', self._out_ws)

        # Plot spectra plots
        if self._plot == 'Spectra' or self._plot == 'Both':
            if isinstance(mtd[self._out_ws], WorkspaceGroup):
                for ws_name in mtd[self._out_ws].getNames():
                    self._plot_spectra(ws_name)
            else:
                self._plot_spectra(self._out_ws)

        # Plot contour plot
        if self._plot == 'Contour' or self._plot == 'Both':
            self._mtd_plot.plot2D(self._out_ws)


    def _setup(self):
        """
        Gets algorithm properties.
        """

        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value

        self._symmetrise = self.getProperty('SymmetriseEnergy').value

        emax_str = self.getPropertyValue('MaxEnergy')
        self._emax = None
        if emax_str != '':
            self._emax = float(emax_str)

        self._res_ws = self.getPropertyValue('Resolution')
        self._out_ws = self.getPropertyValue('OutputWorkspace')


    def _create_res_ws(self, num_sample_hist):
        """
        Creates a resolution workspace.

        @param num_sample_hist Number of histgrams required in workspace
        @returns The generated resolution workspace
        """

        num_res_hist = mtd[self._res_ws].getNumberHistograms()

        logger.notice('Creating resolution workspace.')
        logger.information('Sample has %d spectra\nResolution has %d spectra'
                           % (num_sample_hist, num_res_hist))

        # If the sample workspace has more spectra than the resolution then copy the first spectra
        # to make a workspace with equal spectra count to sample
        if num_sample_hist > num_res_hist:
            logger.information('Copying first resolution spectra for convolution')

            res_ws_list = []
            for _ in range(0, num_sample_hist):
                res_ws_list.append(self._res_ws)

            res_ws_str_list = ','.join(res_ws_list)
            resolution_ws = ConjoinSpectra(res_ws_str_list, 0)

        # If sample has less spectra then crop the resolution to the same number of spectra as
        # resolution
        elif num_sample_hist < num_res_hist:
            logger.information('Cropping resolution workspace to sample')

            resolution_ws = CropWorkspace(InputWorkspace=self._res_ws,
                                          StartWorkspaceIndex=0,
                                          EndWorkspaceIndex=num_sample_hist)

        # If the spectra counts match then just use the resolution as it is
        else:
            logger.information('Using resolution workspace as is')

            resolution_ws = CloneWorkspace(self._res_ws)

        return resolution_ws


    def _convolve_with_res(self, resolution_ws, function_ws_name):
        """
        Performs convolution with an instrument resolution workspace.

        @param resolution_ws The resolution workspace to convolve with
        @param function_ws_name The workspace name for the function to convolute
        """

        logger.notice('Convoluting sample %s with resolution %s'
                      % (function_ws_name, resolution_ws))

        # Convolve the symmetrised sample with the resolution
        ConvolveWorkspaces(OutputWorkspace=function_ws_name,
                           Workspace1=function_ws_name,
                           Workspace2=resolution_ws)


    def _plot_spectra(self, ws_name):
        """
        Plots up to the first 10 spectra from a workspace.

        @param ws_name Name of workspace to plot
        """

        num_hist = mtd[ws_name].getNumberHistograms()

        # Limit number of plotted histograms to 10
        if num_hist > 10:
            num_hist = 10

        # Build plot list
        plot_list = []
        for i in range(0, num_hist):
            plot_list.append(i)

        self._mtd_plot.plotSpectrum(ws_name, plot_list)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(MolDyn)
