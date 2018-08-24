#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

import numpy
from mantid.api import (PythonAlgorithm, AlgorithmFactory,
                        WorkspaceUnitValidator,IEventWorkspaceProperty,
                        WorkspaceProperty)
from mantid.kernel import (Direction, CompositeValidator, Property,
                           CompositeRelation, FloatMandatoryValidator)

class CropWorkspaceForMDNorm(PythonAlgorithm):
    """
    Crops an event workspace and store the information
    about trajectories limits in the run object
    """
    
    
    def category(self):
        """ 
        Return category
        """
        return "Utility\\Workspaces;MDAlgorithms\\Normalisation"

    def name(self):
        """ 
        Return name
        """
        return "CropWorkspaceForMDNorm"

    #TODO: add here the new MDNorm and related
    #def seeAlso(self):
    #    return []

    def summary(self):
        return "Crops an event workspace and store the information"+\
               " about trajectories limits in the run object."

    def PyInit(self):
        """ 
        Declare properties
        """
        allowed_units = CompositeValidator([WorkspaceUnitValidator("DeltaE"),
                                            WorkspaceUnitValidator("Momentum")],
                                           relation=CompositeRelation.OR)
        self.declareProperty(IEventWorkspaceProperty("InputWorkspace", "", 
                                                     direction=Direction.Input,
                                                     validator=allowed_units),
                             doc="Input workspace. It has to be an event workspace"+
                                 " with units of energy transfer or momentum")
        self.declareProperty(name="XMin", defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             validator=FloatMandatoryValidator(),
                             doc="Minimum energy transfer or momentum")
        self.declareProperty(name="XMax", defaultValue=Property.EMPTY_DBL,
                             direction=Direction.Input,
                             validator=FloatMandatoryValidator(),
                             doc="Maximum energy transfer or momentum")
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output), 
                             doc='Output workspace')                     

    def PyExec(self):
        xmin = self.getProperty('XMin').value
        xmax = self.getProperty('XMax').value
        # rebin
        rebin_alg = self.createChildAlgorithm("Rebin", enableLogging=False)
        rebin_alg.setProperty("InputWorkspace",self.getProperty('InputWorkspace').value)
        rebin_alg.setProperty("Params", "{0},{1},{2}".format(xmin,xmax-xmin,xmax))
        rebin_alg.setProperty("PreserveEvents", True)
        rebin_alg.execute()
        out_ws = rebin_alg.getProperty("OutputWorkspace").value
        # crop
        crop_alg = self.createChildAlgorithm("CropWorkspace", enableLogging=False)
        crop_alg.setProperty("InputWorkspace", out_ws)
        crop_alg.setProperty("XMin", xmin)
        crop_alg.setProperty("XMax", xmax)
        crop_alg.execute()
        out_ws = crop_alg.getProperty("OutputWorkspace").value
        # add logs
        num_spectra = out_ws.getNumberHistograms()
        run_obj = out_ws.run()
        min_values = [xmin]*num_spectra
        max_values = [xmax]*num_spectra
        if run_obj.hasProperty('MDNorm_low'):
            min_values = numpy.maximum(min_values, run_obj.getProperty('MDNorm_low').value).tolist()
        run_obj.addProperty('MDNorm_low', min_values, True)
        if run_obj.hasProperty('MDNorm_high'):
            min_values = numpy.minimum(max_values, run_obj.getProperty('MDNorm_high').value).tolist()
        run_obj.addProperty('MDNorm_high', [xmax]*num_spectra, True)
        self.setProperty('OutputWorkspace', out_ws)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(CropWorkspaceForMDNorm)

