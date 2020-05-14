# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
import copy
import json
import numpy as np

from refl1d.names import \
    Experiment, \
    Parameter, \
    QProbe, \
    Slab, \
    SLD

from mantid.api import *
from mantid.simpleapi import *
from mantid.kernel import *


# Unable to generate this props list in PyInit using AlgorithmManager
# from LiquidsReflectometryReduction to copy properties here
LR_ALG_FOR_PROPS = "LiquidsReflectometryReduction"
PROPS_TO_COPY = [
    'RunNumbers',
    'InputWorkspace',
    'NormalizationRunNumber',
    'SignalPeakPixelRange',
    'SubtractSignalBackground',
    'SignalBackgroundPixelRange',
    'NormFlag',
    'NormPeakPixelRange',
    'SubtractNormBackground',
    'NormBackgroundPixelRange',
    'LowResDataAxisPixelRangeFlag',
    'LowResDataAxisPixelRange',
    'LowResNormAxisPixelRangeFlag',
    'LowResNormAxisPixelRange',
    'TOFRange',
    'TOFRangeFlag',
    'QMin',
    'QStep',
    'AngleOffset',
    'AngleOffsetError',
    'OutputWorkspace',
    'ApplyScalingFactor',
    'ScalingFactorFile',
    'SlitTolerance',
    'SlitsWidthFlag',
    'IncidentMediumSelected',
    'GeometryCorrectionFlag',
    'FrontSlitName',
    'BackSlitName',
    'TOFSteps',
    'CropFirstAndLastPoints',
    'ApplyPrimaryFraction',
    'PrimaryFractionRange']

class LRReductionWithReference(DataProcessorAlgorithm):
    def category(self):
        return "Reflectometry\\SNS"

    def name(self):
        return "LRReductionWithReference"

    def version(self):
        return 1

    def summary(self):
        return "REFL reduction using a reference measurement for normalization"

    def PyInit(self):
        self.copyProperties(LR_ALG_FOR_PROPS, PROPS_TO_COPY)
        self.declareProperty("Refl1DModelParameters", "",
            doc="JSON string for Refl1D theoretical model paramters")

    def PyExec(self):
        # Get properties we copied to run LiquidsReflectometryReduction algorithms
        kwargs = dict()
        for prop in PROPS_TO_COPY:
            kwargs[prop] = self.getProperty(prop).value

        # Process the reference normalization run
        norm_kwargs = copy.deepcopy(kwargs)
        norm_kwargs['SignalPeakPixelRange'] = kwargs['NormPeakPixelRange']
        norm_kwargs['SubtractSignalBackground'] = kwargs['SubtractNormBackground']
        norm_kwargs['SignalBackgroundPixelRange'] = kwargs['NormBackgroundPixelRange']
        norm_kwargs['NormFlag'] = False
        norm_kwargs['LowResDataAxisPixelRangeFlag'] = kwargs['LowResNormAxisPixelRangeFlag']
        norm_kwargs['LowResDataAxisPixelRange'] = kwargs['LowResNormAxisPixelRange']
        norm_kwargs['ApplyScalingFactor'] = False
        norm_wksp = LiquidsReflectometryReduction(**norm_kwargs)

        print("Norm:", len(norm_wksp.readX(0)))
        for x, y in zip(norm_wksp.readX(0), norm_wksp.readY(0)):
            print('  ', x, y)

        # Calculate the theoretical reflectivity for normalization using Refl1D
        q = norm_wksp.readX(0)
        model_json = self.getProperty("Refl1DModelParameters").value
        model_dict = json.loads(model_json)
        model_reflectivity = self.calculate_reflectivity(model_dict, q)

        model_wksp = CreateWorkspace(
            DataX=q,
            DataY=model_reflectivity,
            DataE=np.zeros(len(q)),
            UnitX=norm_wksp.getAxis(0).getUnit().unitID())

        print("Model:", len(model_wksp.readX(0)))
        for x, y in zip(model_wksp.readX(0), model_wksp.readY(0)):
            print('  ', x, y)

        # Calculate the incident flux ( measured / model) for reference
        incident_flux = Divide(norm_wksp, model_wksp)

        print("Incident Flux:", len(incident_flux.readX(0)))
        for x, y in zip(incident_flux.readX(0), incident_flux.readY(0)):
            print('  ', x, y)

        # Process the sample run(s)
        sample_kwargs = copy.deepcopy(kwargs)
        sample_kwargs['NormFlag'] = False
        sample_kwargs['ApplyScalingFactor'] = False
        sample_wksp = LiquidsReflectometryReduction(**sample_kwargs)

        # Normalize using the incident flux
        #incident_flux = RebinToWorkspace(incident_flux, sample_wksp)
        print("Sample:", len(sample_wksp.readX(0)))
        for x, y in zip(sample_wksp.readX(0), sample_wksp.readY(0)):
            print('  ', x, y)

        print("Sample | Inciden Flux:")
        for x1, y1, x2, y2 in zip(sample_wksp.readX(0), sample_wksp.readY(0), incident_flux.readX(0), incident_flux.readY(0)):
            print('  ', x1, y1, ' | ', x2, y2)

        out_wksp = Divide(sample_wksp, incident_flux)

        print("\n\nOutputWorkspace # of histograms:", out_wksp.getNumberHistograms())
        print("OutputWorkspace:")
        for x, y in zip(out_wksp.readX(0), out_wksp.readY(0)):
            print('  ', x, y)

        self.setProperty('OutputWorkspace', out_wksp)

    def calculate_reflectivity(self, model_description, q, q_resolution=0.025):
        """
        Reflectivity calculation using refl1d

        :param model_description: dict that holds parameters for the
                                  theoretical Refl1D model.

            Example dict for paramters:
            {
                'back_sld': 2.07,
                'back_roughness': 1.0,
                'front_sld': 0,
                'scale': 1,
                'background': 0
                'layers': [{
                    'thickness': 10,
                    'sld': 3.5,
                    'isld': 0,
                    'roughness': 2}]
            }
        :param q: Momentum transfer (Q) to use to calculate the model
        :param q_resolution: Momentum transfer resolution to multiply by Q
        :return: Calculate reflectivity of the theoretical model
        """
        zeros = np.zeros(len(q))
        dq = q_resolution * q
        # The QProbe object represents the beam
        probe = QProbe(q, dq, data=(zeros, zeros))
        sample = Slab(material=SLD(name='back',
                                   rho=model_description['back_sld']),
                                   interface=model_description['back_roughness'])
        # Add each layer
        for i, layer in enumerate(model_description['layers']):
            sample = sample | Slab(material=SLD(name='layer%s' % i,
                                                rho=layer['sld'],
                                                irho=layer['isld']),
                                                thickness=layer['thickness'],
                                                interface=layer['roughness'])
        sample = sample | Slab(material=SLD(name='front',
                                            rho=model_description['front_sld']))
        probe.background = Parameter(value=model_description['background'], name='background')
        expt = Experiment(probe=probe, sample=sample)
        q, r = expt.reflectivity()
        return model_description['scale'] * r

AlgorithmFactory.subscribe(LRReductionWithReference)
