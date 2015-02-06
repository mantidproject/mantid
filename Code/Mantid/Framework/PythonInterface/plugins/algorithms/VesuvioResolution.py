from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import mantid


class VesuvioResolution(PythonAlgorithm):

    def category(self):
        return 'Inelastic'

    def summary(self):
        return 'Calculates the resolution function for VESUVIO'

    def PyInit(self):
        self.declareProperty(WorkspaceProperty(name='Workspace',
                                               defaultValue='',
                                               direction=Direction.Input),
                             doc='Sample workspace')

        self.declareProperty(name='SpectrumIndex', defaultValue=0,
                             doc='Spectra index to use for resolution')

        self.declareProperty(name='Mass', defaultValue=100.0,
                             doc='The mass defining the recoil peak in AMU')

        self.declareProperty(WorkspaceProperty(name='OutputWorkspaceTOF',
                                               defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output resolution workspace in TOF')

        self.declareProperty(WorkspaceProperty(name='OutputWorkspaceYSpace',
                                               defaultValue='',
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc='Output resolution workspace in ySpace')


    def validateInputs(self):
        """
        Does basic validation for inputs.
        """

        issues = dict()

        sample_ws = self.getProperty('Workspace').value
        spectrum_index = self.getProperty('SpectrumIndex').value

        if spectrum_index > sample_ws.getNumberHistograms() - 1:
            issues['SpectrumIndex'] = 'Spectrum index is out of range'

        out_ws_tof = self.getPropertyValue('OutputWorkspaceTOF')
        out_ws_ysp = self.getPropertyValue('OutputWorkspaceYSpace')

        output_tof = (out_ws_tof != '')
        output_ysp = (out_ws_ysp != '')

        if not (output_tof or output_ysp):
            warning_message = 'Must output in either time of flight or ySpace'
            issues['OutputWorkspaceTOF'] = warning_message
            issues['OutputWorkspaceYSpace'] = warning_message

        return issues


    def PyExec(self):
        sample_ws = self.getProperty('Workspace').value
        out_ws_tof = self.getPropertyValue('OutputWorkspaceTOF')
        out_ws_ysp = self.getPropertyValue('OutputWorkspaceYSpace')
        self._spectrum_index = self.getProperty('SpectrumIndex').value
        self._mass = self.getProperty('Mass').value

        output_tof = (out_ws_tof != '')
        output_ysp = (out_ws_ysp != '')

        if output_tof:
            res_tof = self._calculate_resolution(sample_ws, out_ws_tof)
            self.setProperty('OutputWorkspaceTOF', res_tof)

        if output_ysp:
            y_space_conv = mantid.api.AlgorithmManager.createUnmanaged('ConvertToYSpace')
            y_space_conv.initialize()
            y_space_conv.setChild(True)
            y_space_conv.setAlwaysStoreInADS(True)
            y_space_conv.setProperty('InputWorkspace', sample_ws)
            y_space_conv.setProperty('OutputWorkspace', '__yspace_sample')
            y_space_conv.setProperty('Mass', self._mass)
            y_space_conv.execute()

            res_ysp = self._calculate_resolution(mtd['__yspace_sample'], out_ws_ysp)
            self.setProperty('OutputWorkspaceYSpace', res_ysp)
            DeleteWorkspace('__yspace_sample')


    def _calculate_resolution(self, workspace, output_ws_name):
        """
        Calculates the resolution function using the VesuvioResolution fit function.

        @param workspace The sample workspace
        @param output_ws_name Name of the output workspace
        """

        function = 'name=VesuvioResolution, Mass=%f' % self._mass
        fit_naming_stem = '__vesuvio_res_fit'

        # Execute the resolution function using fit.
        # Functions can't currently be executed as stand alone objects,
        # so for now we will run fit with zero iterations to achieve the same result.
        fit = mantid.api.AlgorithmManager.createUnmanaged('Fit')
        fit.initialize()
        fit.setChild(True)
        mantid.simpleapi._set_properties(fit, function, InputWorkspace=workspace, MaxIterations=0,
                CreateOutput=True, Output=fit_naming_stem, WorkspaceIndex=self._spectrum_index,
                OutputCompositeMembers=True)
        fit.execute()
        fit_ws = fit.getProperty('OutputWorkspace').value

        # Extract just the function values from the fit spectrum
        extract = mantid.api.AlgorithmManager.createUnmanaged('ExtractSingleSpectrum')
        extract.initialize()
        extract.setChild(True)
        extract.setProperty('InputWorkspace', fit_ws)
        extract.setProperty('OutputWorkspace', output_ws_name)
        extract.setProperty('WorkspaceIndex', 1)
        extract.execute()

        res_ws = extract.getProperty('OutputWorkspace').value
        return res_ws


AlgorithmFactory.subscribe(VesuvioResolution)
