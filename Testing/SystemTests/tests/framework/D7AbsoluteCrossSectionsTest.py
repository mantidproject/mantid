# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid.simpleapi import *


class ILL_D7_Powder_Test(systemtesting.MantidSystemTest):

    def __init__(self):
        super(ILL_D7_Powder_Test, self).__init__()
        self.setUp()

    def setUp(self):
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'D7'
        config.appendDataSearchSubDir('ILL/D7/')

    def cleanup(self):
        mtd.clear()

    def validate(self):
        self.tolerance = 1e-3
        self.tolerance_is_rel_err = True
        self.disableChecking = ['Instrument']
        return ['normalized_powder_XYZ', 'ILL_D7_Powder_6p.nxs']

    def runTest(self):
        vanadium_mass = 8.535
        sample_formula_mass = 137.33 * 2.0 + 54.93 + 127.6 + 15.999 * 6.0
        sample_mass = 7.83

        vanadium_dictionary = {'SampleMass': vanadium_mass, 'FormulaUnitMass': 50.9412,
                               'SampleChemicalFormula': 'V', 'SampleDensity': 6.1,
                               'Height': 1.0, 'ElementSize':0.5,
                               'SampleInnerRadius': 0.91, 'SampleOuterRadius': 0.99,
                               'ContainerChemicalFormula': 'Al', 'ContainerDensity': 2.7,
                               'ContainerOuterRadius': 1.0, 'ContainerInnerRadius': 0.9}

        sample_dictionary = {'SampleMass': sample_mass, 'FormulaUnitMass': sample_formula_mass}

        calibration_file = "D7_YIG_calibration.xml"

        # Empty container
        PolDiffILLReduction(
            Run='450747:450748',
            OutputWorkspace='container_ws',
            ProcessAs='Empty'
        )

        # Absorber
        PolDiffILLReduction(
            Run='450758:450759',
            OutputWorkspace='absorber_ws',
            ProcessAs='Cadmium'
        )

        PolDiffILLReduction(
            Run='450769:450770',
            OutputWorkspace='pol_corrections',
            CadmiumWorkspace='absorber_ws',
            EmptyContainerWorkspace='container_ws',
            Transmission='0.9',
            OutputTreatment='AveragePol',
            ProcessAs='Quartz'
        )

        PolDiffILLReduction(
            Run='450835:450836',
            OutputWorkspace='vanadium_ws',
            CadmiumWorkspace='absorber_ws',
            EmptyContainerWorkspace='container_ws',
            Transmission='0.89',
            QuartzWorkspace='pol_corrections',
            OutputTreatment='Sum',
            SampleGeometry='Annulus',
            SelfAttenuationMethod='Numerical',
            SampleAndEnvironmentProperties=vanadium_dictionary,
            AbsoluteNormalisation=True,
            InstrumentCalibration=calibration_file,
            ProcessAs='Vanadium'
        )

        PolDiffILLReduction(
            Run='451235:451236',
            OutputWorkspace='sample_ws',
            CadmiumWorkspace='absorber_ws',
            EmptyContainerWorkspace='container_ws',
            Transmission='0.91',
            QuartzWorkspace='pol_corrections',
            SelfAttenuationMethod='Transmission',
            OutputTreatment='Individual',
            SampleGeometry='None',
            SampleAndEnvironmentProperties=sample_dictionary,
            InstrumentCalibration=calibration_file,
            ProcessAs='Sample'
        )

        D7AbsoluteCrossSections(
            InputWorkspace='sample_ws',
            OutputWorkspace='normalized_powder_XYZ',
            CrossSectionSeparationMethod='XYZ',
            NormalisationMethod='Vanadium',
            VanadiumInputWorkspace='vanadium_ws',
            OutputTreatment='Merge',
            OutputUnits='TwoTheta',
            ScatteringAngleBinSize=1.5,  # degrees
            SampleAndEnvironmentProperties=sample_dictionary,
            AbsoluteUnitsNormalisation=True,
            ClearCache=True
        )
