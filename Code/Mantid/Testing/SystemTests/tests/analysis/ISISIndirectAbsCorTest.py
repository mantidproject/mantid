import stresstesting
from mantid.simpleapi import *
from IndirectImport import is_supported_f2py_platform
import os

#====================================================================================================


class CylAbsTest(stresstesting.MantidStressTest):

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        import IndirectAbsCor as Main

        sname = 'irs26176_graphite002_red'
        LoadNexusProcessed(Filename=sname, OutputWorkspace=sname)

        beam = [3.0, 1.0, -1.0, 2.0, -2.0, 0.0, 3.0, 0.0, 3.0]
        size = [0.2, 0.25, 0.26, 0.0]
        density = [0.1, 0.1, 0.1]
        sigs = [5.0, 0.1, 0.1]
        siga = [0.0, 5.0, 5.0]
        avar = 0.002
        saveOp = False
        Main.AbsRun(sname, 'cyl', beam, 2, size, density,
                    sigs, siga, avar, saveOp)

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_graphite002_cyl_Abs', 'ISISIndirectAbsCor_CylAbsTest.nxs'

#====================================================================================================


class FltAbsTest(stresstesting.MantidStressTest):

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        import IndirectAbsCor as Main

        sname = 'irs26176_graphite002_red'
        LoadNexusProcessed(Filename=sname, OutputWorkspace=sname)

        beam = ''
        size = [0.1, 0.01, 0.01]
        density = [0.1, 0.1, 0.1]
        sigs = [5.0, 0.1, 0.1]
        siga = [0.0, 5.0, 5.0]
        avar = 45.0
        saveOp = False
        Main.AbsRun(sname, 'flt', beam, 2, size, density,
                    sigs, siga, avar, saveOp)

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_graphite002_flt_Abs', 'ISISIndirectAbsCor_FltAbsTest.nxs'


#====================================================================================================


class FltAbsTSecCloseTo90Test(stresstesting.MantidStressTest):

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        import IndirectAbsCor as Main

        sname = 'irs59330_graphite002_red'
        LoadNexusProcessed(Filename=sname, OutputWorkspace=sname)

        beam = ''
        size = [0.1, 0.01, 0.01]
        density = [0.05, 0.5, 0.5]
        sigs = [5.0, 0.1, 0.1]
        siga = [0.0, 5.0, 5.0]
        avar = 45.0
        saveOp = False
        Main.AbsRun(sname, 'flt', beam, 2, size, density,
                    sigs, siga, avar, saveOp)

    def validate(self):
        self.tolerance = 1e-3
        return 'iris59330_graphite002_flt_Abs', 'ISISIndirectAbsCor_FltAbsTSecCloseTo90Test.nxs'

#====================================================================================================


class AbsRunFeederTest(stresstesting.MantidStressTest):
    """
    Test AbsRunFeeder with given values for scattering and absorption cross sections
    for both sample and can.
    """

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        from IndirectAbsCor import AbsRunFeeder

        # H20 sample
        inputWS = 'irs26176_graphite002_red'
        # cylindrical Vanadium can
        canWS = 'irs26173_graphite002_red'

        Load(inputWS + '.nxs', OutputWorkspace=inputWS)
        Load(canWS + '.nxs', OutputWorkspace=canWS)

        geom = 'cyl'
        ncan = 2
        size = [0.2, 0.25, 0.26, 0.0]
        sigs = [5.0, 0.1, 0.1]
        siga = [0.0, 5.0, 5.0]
        avar = 0.002
        density = [0.1, 0.1, 0.1]
        beam_width = 4.0
        AbsRunFeeder(inputWS, canWS, geom, ncan, size, avar, density, beam_width=beam_width, sigs=sigs, siga=siga)

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_graphite002_cyl_Abs', 'ISISIndirectAbsCor_AbsRunFeederTest.nxs'

#====================================================================================================


class AbsRunFeederChemicalFormulaTest(stresstesting.MantidStressTest):
    """
    Test AbsRunFeeder with chemical formula input for scattering and absorption cross sections
    for both sample and can.
    """

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        from IndirectAbsCor import AbsRunFeeder

        # H20 sample
        inputWS = 'irs26176_graphite002_red'
        # cylindrical Vanadium can
        canWS = 'irs26173_graphite002_red'

        Load(inputWS + '.nxs', OutputWorkspace=inputWS)
        Load(canWS + '.nxs', OutputWorkspace=canWS)

        geom = 'cyl'
        ncan = 2
        size = [0.2, 0.25, 0.26, 0.0]
        avar = 0.002
        density = [0.1, 0.1, 0.1]
        beam_width = 4.0
        sampleFormula = 'H2-O'
        canFormula = 'V'
        AbsRunFeeder(inputWS, canWS, geom, ncan, size, avar, density, beam_width=beam_width, sample_formula=sampleFormula, can_formula=canFormula,  sigs=[0,0,0], siga=[0,0,0])

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_graphite002_cyl_Abs', 'ISISIndirectAbsCor_ChemicalFormulaTest.nxs'

#====================================================================================================


class AbsRunFeederDefaultBeamWidthTest(stresstesting.MantidStressTest):
    """
    Test AbsRunFeeder with given values for scattering and absorption cross sections
    for both sample and can and the beam width taken from the IPF.
    """

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        from IndirectAbsCor import AbsRunFeeder

        # H20 sample
        inputWS = 'irs26176_graphite002_red'
        # cylindrical Vanadium can
        canWS = 'irs26173_graphite002_red'

        Load(inputWS + '.nxs', OutputWorkspace=inputWS)
        path = os.path.join(config['instrumentDefinition.directory'], 'IRIS_Parameters.xml')
        LoadParameterFile(inputWS, Filename=path)
        Load(canWS + '.nxs', OutputWorkspace=canWS)

        geom = 'cyl'
        ncan = 2
        size = [0.2, 0.25, 0.26, 0.0]
        sigs = [5.0, 0.1, 0.1]
        siga = [0.0, 5.0, 5.0]
        avar = 0.002
        density = [0.1, 0.1, 0.1]
        AbsRunFeeder(inputWS, canWS, geom, ncan, size, avar, density, sigs=sigs, siga=siga)

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_graphite002_cyl_Abs', 'ISISIndirectAbsCor_DefaultBeamWidthTest.nxs'

#====================================================================================================


class AbsRunFeederDiffractionTest(stresstesting.MantidStressTest):
    """
    Test AbsRunFeeder with sample and can material formulas for a diffraction run.
    """

    def skipTests(self):
        return not is_supported_f2py_platform()

    def runTest(self):
        from IndirectAbsCor import AbsRunFeeder

        # H20 sample
        inputWS = 'irs26176_diffspec_red'
        # cylindrical Vanadium can
        canWS = 'irs26173_diffspec_red'

        Load(inputWS + '.nxs', OutputWorkspace=inputWS)
        Load(canWS + '.nxs', OutputWorkspace=canWS)

        geom = 'cyl'
        ncan = 2
        size = [0.2, 0.25, 0.26, 0.0]
        avar = 0.002
        density = [0.1, 0.1, 0.1]
        beam_width = 4.0
        sampleFormula = 'H2-O'
        canFormula = 'V'
        AbsRunFeeder(inputWS, canWS, geom, ncan, size, avar, density, beam_width=beam_width, sample_formula=sampleFormula, can_formula=canFormula,  sigs=[0,0,0], siga=[0,0,0])

    def validate(self):
        self.tolerance = 1e-3
        return 'irs26176_diffspec_cyl_Abs', 'ISISIndirectAbsCor_AbsRunFeederDiffractionTest.nxs'
