# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)

from mantid.api import (PythonAlgorithm, AlgorithmFactory,
                        WorkspaceUnitValidator, MatrixWorkspaceProperty,
                        WorkspaceProperty, WorkspaceFactory)
from mantid.kernel import (Direction, StringListValidator, logger)

from pystog.converter import Converter

SQ = 'S(Q)'
FQ = 'F(Q)'
FKQ = 'FK(Q)'
DCS = 'DCS(Q)'


class PDConvertReciprocalSpace(PythonAlgorithm):
    """
    Convert between different reciprocal space functions
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
        return "PDConvertReciprocalSpace"

    def seeAlso(self):
        return ["PDConvertRealSpace"]

    def summary(self):
        return "Transforms a Workspace2D between different " + \
               "reciprocal space functions."

    def PyInit(self):
        """
        Declare properties
        """
        allowed_units = WorkspaceUnitValidator("MomentumTransfer")
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "",
                                                     direction=Direction.Input,
                                                     validator=allowed_units),
                             doc="Input workspace with units of momentum transfer")
        functions = [SQ, FQ, FKQ, DCS]
        self.declareProperty("From", SQ, StringListValidator(functions), "Function type in the input workspace")
        self.declareProperty("To", SQ, StringListValidator(functions), "Function type in the output workspace")
        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace')

    def validateInputs(self):
        result = dict()
        input_ws = self.getProperty('InputWorkspace').value
        from_quantity = self.getProperty('From').value
        to_quantity = self.getProperty('To').value

        if from_quantity != to_quantity:  # noop doesn't require material
            requireMaterial = True
            if from_quantity == SQ and to_quantity == FQ:
                requireMaterial = False
            elif from_quantity == FQ and to_quantity == SQ:
                requireMaterial = False
            if requireMaterial:
                if not input_ws.sample().getMaterial():
                    result['InputWorkspace'] = 'Please run SetSample or SetSampleMaterial'
                else:
                    cohScattSqrd = input_ws.sample().getMaterial().cohScatterLengthSqrd()
                    if cohScattSqrd == 0.:
                        if from_quantity in [SQ, FQ] and to_quantity in [DCS, FKQ]:
                            result['To'] = 'Require non-zero coherent scattering length'
                        if to_quantity in [SQ, FQ] and from_quantity in [DCS, FKQ]:
                            result['From'] = 'Require non-zero coherent scattering length'
        return result

    def PyExec(self):
        input_ws = self.getProperty("InputWorkspace").value
        output_ws_name = self.getProperty('OutputWorkspace').valueAsStr
        from_quantity = self.getProperty("From").value
        to_quantity = self.getProperty("To").value

        if input_ws.name() == output_ws_name:
            output_ws = input_ws
        else:
            output_ws = WorkspaceFactory.create(input_ws)

        self.setProperty('OutputWorkspace', output_ws)
        if from_quantity == to_quantity:
            logger.warning('The input and output functions are the same. Nothing to be done')
            return
        c = Converter()
        transformation = {SQ: {FQ: c.S_to_F, FKQ: c.S_to_FK, DCS: c.S_to_DCS},
                          FQ: {SQ: c.F_to_S, FKQ: c.F_to_FK, DCS: c.F_to_DCS},
                          FKQ: {SQ: c.FK_to_S, FQ: c.FK_to_F, DCS: c.FK_to_DCS},
                          DCS: {SQ: c.DCS_to_S, FQ: c.DCS_to_F, FKQ: c.DCS_to_FK}}

        if input_ws.sample().getMaterial():
            sample_kwargs = {"<b_coh>^2": input_ws.sample().getMaterial().cohScatterLengthSqrd(),
                             "<b_tot^2>": input_ws.sample().getMaterial().totalScatterLengthSqrd(),
                             "rho": input_ws.sample().getMaterial().numberDensity}
        else:
            sample_kwargs = dict()

        for sp_num in range(input_ws.getNumberHistograms()):
            x = input_ws.readX(sp_num)
            output_ws.setX(sp_num, x)
            y = input_ws.readY(sp_num)
            e = input_ws.readE(sp_num)
            if len(x) == len(y) + 1:
                x = 0.5 * (x[:-1] + x[1:])

            new_y, new_e = transformation[from_quantity][to_quantity](x, y, e, **sample_kwargs)
            output_ws.setY(sp_num, new_y)
            output_ws.setE(sp_num, new_e)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(PDConvertReciprocalSpace)
