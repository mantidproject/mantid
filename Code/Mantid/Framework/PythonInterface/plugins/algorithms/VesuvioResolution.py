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
        self.declareProperty(WorkspaceProperty(name='Sample',
                                               defaultValue='',
                                               direction=Direction.Input),
                             doc='Sample workspace')

        self.declareProperty(name='SpectraIndex', defaultValue=0,
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
        sample_ws = self.getPropertyValue('Sample')
        out_ws_tof = self.getPropertyValue('OutputWorkspaceTOF')
        out_ws_ysp = self.getPropertyValue('OutputWorkspaceYSpace')
        spectrum_index = self.getProperty('SpectraIndex').value
        mass = self.getProperty('Mass').value

        output_tof = (out_ws_tof != '')
        output_ysp = (out_ws_ysp != '')

        # Give a dummy name here since we will need resolution in time of flight
        # in order to convert to ySpace
        if not output_tof:
            out_ws_tof = '__res_tof'

        function = 'name=VesuvioResolution, Mass=%f' % mass
        fit_naming_stem = '__vesuvio_res_fit'

        # Execute the resolution function using fit.
        # Functions can't currently be executed as stand alone objects,
        # so for now we will run fit with zero iterations to achieve the same result.
        fit = mantid.api.AlgorithmManager.createUnmanaged('Fit')
        fit.initialize()
        fit.setChild(True)
        fit.setLogging(False)
        mantid.simpleapi._set_properties(fit, function, sample_ws, MaxIterations=0,
                CreateOutput=True, Output=fit_naming_stem, WorkspaceIndex=spectrum_index,
                OutputCompositeMembers=True)
        fit.execute()
        fit_ws = fit.getProperty("OutputWorkspace").value

        # Extract just the function values from the fit spectrum
        extract = mantid.api.AlgorithmManager.createUnmanaged('ExtractSingleSpectrum')
        extract.initialize()
        extract.setChild(True)
        extract.setLogging(False)
        extract.setProperty("InputWorkspace", fit_ws)
        extract.setProperty("OutputWorkspace", out_ws_tof)
        extract.setProperty("WorkspaceIndex", 1)
        extract.execute()
        res_tof = extract.getProperty('OutputWorkspace').value

        if output_tof:
            self.setProperty('OutputWorkspaceTOF', res_tof)

        # Convert to y-Space if needed
        if output_ysp:

            # Clone the raw workspace to use the instrument parameters for ySpace conversion
            # taking only the spectra that was used for the time of flight resolution
            extract2 = mantid.api.AlgorithmManager.createUnmanaged('ExtractSingleSpectrum')
            extract2.initialize()
            extract2.setChild(True)
            extract2.setLogging(False)
            extract2.setProperty("InputWorkspace", sample_ws)
            extract2.setProperty("OutputWorkspace", '__raw_clone')
            extract2.setProperty("WorkspaceIndex", spectrum_index)
            extract2.execute()
            raw_clone = extract2.getProperty('OutputWorkspace').value

            # Get the resolution data in time of flight from the fit workspace
            res_data_x = res_tof.dataX(0)
            res_data_y = res_tof.dataY(0)
            res_data_e = res_tof.dataE(0)

            # Copy it to the cloned raw workspace spectrum
            raw_clone.setX(0, res_data_x)
            raw_clone.setY(0, res_data_y)
            raw_clone.setE(0, res_data_e)

            # Convert the cloned workspace to ySpace
            extract2 = mantid.api.AlgorithmManager.createUnmanaged('ConvertToYSpace')
            extract2.initialize()
            extract2.setChild(True)
            extract2.setLogging(False)
            extract2.setProperty("InputWorkspace", raw_clone)
            extract2.setProperty("OutputWorkspace", out_ws_ysp)
            extract2.setProperty("Mass", mass)
            extract2.execute()
            out_ws_ysp = extract2.getProperty('OutputWorkspace').value

            self.setProperty('OutputWorkspaceYSpace', out_ws_ysp)


AlgorithmFactory.subscribe(VesuvioResolution)
