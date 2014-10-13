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

        self.declareProperty(name='OutputMode', defaultValue='TOF',
                             validator=StringListValidator(['TOF', 'y-Space']),
                             doc='')

        self.declareProperty(WorkspaceProperty(name='OutputWorkspace',
                                               defaultValue='',
                                               direction=Direction.Output),
                             doc='Output resolution workspace')


    def PyExec(self):
        sample_ws = self.getPropertyValue('Sample')
        out_ws = self.getPropertyValue('OutputWorkspace')
        spectrum_index = self.getProperty('SpectraIndex').value
        mass = self.getProperty('Mass').value
        output_mode = self.getPropertyValue('OutputMode')

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
        extract.setProperty("OutputWorkspace", out_ws)
        extract.setProperty("WorkspaceIndex", 1)
        extract.execute()
        out_ws = extract.getProperty('OutputWorkspace').value

        # Convert to y-Space if needed
        if output_mode == 'y-Space':
            pass

        self.setProperty('OutputWorkspace', out_ws)


AlgorithmFactory.subscribe(VesuvioResolution)
