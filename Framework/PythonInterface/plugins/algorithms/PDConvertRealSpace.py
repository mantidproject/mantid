# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import (PythonAlgorithm, AlgorithmFactory,
                        MatrixWorkspaceProperty,
                        WorkspaceProperty, WorkspaceFactory)
from mantid.kernel import (Direction, StringListValidator, logger)

from pystog.converter import Converter


class PDConvertRealSpace(PythonAlgorithm):
    """
    Convert between different real space functions
    """
    def category(self):
        """
        Return category
        """
        return "Diffraction\\Utility"

    def name(self):
        """
        Return name
        """
        return "PDConvertRealSpace"

    def seeAlso(self):
        return ["PDConvertReciprocalSpace"]

    def summary(self):
        return "Transforms a Workspace2D between different "+\
               "real space functions."

    def PyInit(self):
        """
        Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input),
                             doc="Input workspace. The units are assumed to be distance")
        functions=["G(r)","GK(r)","g(r)"]
        self.declareProperty("From", 'G(r)', StringListValidator(functions),
                             "Function type in the input workspace")
        self.declareProperty("To", 'G(r)', StringListValidator(functions),
                             "Function type in the output workspace")
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace')

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        output_ws_name = self.getProperty('OutputWorkspace').valueAsStr
        from_quantity = self.getProperty("From").value
        to_quantity = self.getProperty("To").value

        if(input_ws.name()==output_ws_name):
            output_ws=input_ws
        else:
            output_ws=WorkspaceFactory.create(input_ws)

        self.setProperty('OutputWorkspace', output_ws)
        if(from_quantity==to_quantity):
            logger.warning('The input and output functions are the same. Nothing to be done')
            return
        c=Converter()
        transformation={'G(r)':{'GK(r)':c.G_to_GK, 'g(r)':c.G_to_g},
                        'GK(r)':{'G(r)':c.GK_to_G, 'g(r)':c.GK_to_g},
                        'g(r)':{'G(r)':c.g_to_G, 'GK(r)':c.g_to_GK}}
        sample_kwargs={"<b_coh>^2":input_ws.sample().getMaterial().cohScatterLengthSqrd(),
                       "<b_tot^2>":input_ws.sample().getMaterial().totalScatterLengthSqrd(),
                       "rho":input_ws.sample().getMaterial().numberDensity}
        if ((sample_kwargs["<b_coh>^2"]<=0) or (sample_kwargs["<b_tot^2>"]<=0) or
           (sample_kwargs["rho"]<=0)):
            raise RuntimeError('Please run SetSampleMaterial algorithm before running'+
                               ' this algorithm')
        for sp_num in range(input_ws.getNumberHistograms()):
            x = input_ws.readX(sp_num)
            output_ws.setX(sp_num,x)
            y = input_ws.readY(sp_num)
            e = input_ws.readE(sp_num)
            if len(x)==len(y)+1:
                x = 0.5*(x[:-1]+x[1:])

            new_y, new_e=transformation[from_quantity][to_quantity](x,y,e,**sample_kwargs)
            output_ws.setY(sp_num,new_y)
            output_ws.setE(sp_num,new_e)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(PDConvertRealSpace)
