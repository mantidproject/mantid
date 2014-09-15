from mantid.simpleapi import *
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, PropertyMode
from mantid.kernel import StringListValidator, StringMandatoryValidator, Direction, logger
from mantid import config
import math
import os


class Fury(PythonAlgorithm):

    def category(self):
        return "Workflow\\MIDAS;PythonAlgorithms"

    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty('Sample', '',
                             optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc="Name for the Sample workspace.")

        self.declareProperty(MatrixWorkspaceProperty('Resolution', '',
                             optional=PropertyMode.Mandatory, direction=Direction.Input),
                             doc="Name for the Resolution workspace.")

        self.declareProperty(name='EnergyMin', defaultValue=-0.5, doc='Minimum energy for fit. Default=-0.5')
        self.declareProperty(name='EnergyMax', defaultValue=0.5, doc='Maximum energy for fit. Default=0.5')
        self.declareProperty(name='NumBins', defaultValue=10, doc='Binning value (integer) for sample. Default=1')

        self.declareProperty(MatrixWorkspaceProperty('ParameterWorkspace', '',
                             direction=Direction.Output, optional=PropertyMode.Optional),
                             doc='Table workspace for saving Fury properties')

        self.declareProperty(MatrixWorkspaceProperty('OutputWorkspace', '',
                             direction=Direction.Output, optional=PropertyMode.Optional),
                             doc='Output workspace')

        self.declareProperty(name='Plot', defaultValue=False, doc='Switch Plot Off/On')
        self.declareProperty(name='Verbose', defaultValue=False, doc='Switch Verbose Off/On')
        self.declareProperty(name='Save', defaultValue=False, doc='Switch Save result to nxs file Off/On')
        self.declareProperty(name='DryRun', defaultValue=False, doc='Only calculate and output the parameters')


    def PyExec(self):
        self._setup()

        self._calculate_parameters()

        if not self._dry_run:
            self._fury('__Fury_sample_cropped')

            if self._save:
                workdir = config['defaultsave.directory']
                opath = os.path.join(workdir, self._output_workspace + '.nxs')
                SaveNexusProcessed(InputWorkspace=self._output_workspace, Filename=opath)
                if self._verbose:
                    logger.notice('Output file : ' + opath)

            if self._plot:
                self._plot_output()
        else:
            logger.notice('Dry run, will not run Fury')

        DeleteWorkspace('__Fury_sample_cropped')

        self.setProperty('ParameterWorkspace', self._parameter_table)
        self.setProperty('OutputWorkspace', self._output_workspace)


    def _setup(self):
        """
        Gets algorithm properties.
        """

        from IndirectCommon import getWSprefix

        self._sample = self.getPropertyValue('Sample')
        self._resolution = self.getPropertyValue('Resolution')

        self._emin = self.getProperty('EnergyMin').value
        self._emax = self.getProperty('EnergyMax').value
        self._nbin = self.getProperty('NumBins').value

        self._parameter_table = self.getPropertyValue('ParameterWorkspace')
        if self._parameter_table == '':
            self._parameter_table = getWSprefix(self._sample) + 'FuryParameters'

        self._output_workspace = self.getPropertyValue('OutputWorkspace')
        if self._output_workspace == '':
            self._output_workspace = getWSprefix(self._sample) + 'iqt'

        self._verbose = self.getProperty('Verbose').value
        self._plot = self.getProperty('Plot').value
        self._save = self.getProperty('Save').value
        self._dry_run = self.getProperty('DryRun').value


    def _calculate_parameters(self):
        """
        Calculates the Fury parameters and saves in a table workspace.
        """
        param_table = CreateEmptyTableWorkspace(OutputWorkspace=self._parameter_table)

        param_table.addColumn('int', 'NumberInputPoints')
        param_table.addColumn('int', 'NumberBins')
        param_table.addColumn('int', 'NumberOutputPoints')
        param_table.addColumn('float', 'EnergyMin')
        param_table.addColumn('float', 'EnergyMax')
        param_table.addColumn('float', 'EnergyWidth')
        param_table.addColumn('float', 'Resolution')

        CropWorkspace(InputWorkspace=self._sample, OutputWorkspace='__Fury_sample_cropped', Xmin=self._emin, Xmax=self._emax)
        x_data = mtd['__Fury_sample_cropped'].readX(0)
        number_input_points = len(x_data) - 1
        number_points_per_bin = number_input_points / self._nbin
        self._einc = (abs(self._emin) + self._emax) / number_points_per_bin

        # inst = mtd[self._sample].getInstrument()
        # res = inst.getNumberParameter("resolution")[0]
        res = 0.0175
        nres = res / self._einc

        param_table.addRow([number_input_points, self._nbin, number_points_per_bin, self._emin, self._emax, self._einc, res])
        self.setProperty('ParameterWorkspace', param_table)


    def _plot_output(self):
        """
        Plot output.
        """
        from IndirectImport import import_mantidplot
        mtd_plot = import_mantidplot()

        spectra_range = range(0, mtd[self._output_workspace].getNumberHistograms())

        graph = mtd_plot.plotSpectrum(self._output_workspace, spectra_range)

        layer = graph.activeLayer()
        layer.setScale(mtd_plot.Layer.Left, 0, 1.0)


    def _fury(self, sam_workspace):
        """
        Run Fury.
        """
        from IndirectCommon import StartTime, EndTime, CheckHistZero, CheckHistSame, CheckAnalysers

        StartTime('Fury')

        rebin_param = str(self._emin) + ',' + str(self._einc) + ',' + str(self._emax)
        Rebin(InputWorkspace=sam_workspace, OutputWorkspace=sam_workspace, Params=rebin_param, FullBinsOnly=True)

        # Process RES Data Only Once
        CheckAnalysers(sam_workspace, self._resolution, self._verbose)
        nres, _ = CheckHistZero(self._resolution)
        if nres > 1:
            CheckHistSame(sam_workspace, 'Sample', self._resolution, 'Resolution')

        tmp_res_workspace = '__tmp_' + self._resolution
        Rebin(InputWorkspace=self._resolution, OutputWorkspace=tmp_res_workspace, Params=rebin_param, FullBinsOnly=True)
        Integration(InputWorkspace=tmp_res_workspace, OutputWorkspace='res_int')
        ConvertToPointData(InputWorkspace=tmp_res_workspace, OutputWorkspace=tmp_res_workspace)
        ExtractFFTSpectrum(InputWorkspace=tmp_res_workspace, OutputWorkspace='res_fft', FFTPart=2)
        Divide(LHSWorkspace='res_fft', RHSWorkspace='res_int', OutputWorkspace='res')
        Rebin(InputWorkspace=sam_workspace, OutputWorkspace='sam_data', Params=rebin_param)
        Integration(InputWorkspace='sam_data', OutputWorkspace='sam_int')
        ConvertToPointData(InputWorkspace='sam_data', OutputWorkspace='sam_data')
        ExtractFFTSpectrum(InputWorkspace='sam_data', OutputWorkspace='sam_fft', FFTPart=2)
        Divide(LHSWorkspace='sam_fft', RHSWorkspace='sam_int', OutputWorkspace='sam')

        Divide(LHSWorkspace='sam', RHSWorkspace='res', OutputWorkspace=self._output_workspace)

        # Cleanup Sample Files
        DeleteWorkspace('sam_data')
        DeleteWorkspace('sam_int')
        DeleteWorkspace('sam_fft')
        DeleteWorkspace('sam')

        # Crop nonsense values off workspace
        binning = int(math.ceil(mtd[self._output_workspace].blocksize() / 2.0))
        bin_v = mtd[self._output_workspace].dataX(0)[binning]
        CropWorkspace(InputWorkspace=self._output_workspace, OutputWorkspace=self._output_workspace, XMax=bin_v)

        # Clean Up RES files
        DeleteWorkspace(tmp_res_workspace)
        DeleteWorkspace('res_int')
        DeleteWorkspace('res_fft')
        DeleteWorkspace('res')

        EndTime('Fury')


# Register algorithm with Mantid
AlgorithmFactory.subscribe(Fury)
