# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import systemtesting
from mantid import config
from mantid.simpleapi import (
    BASISCrystalDiffraction,
    GroupWorkspaces,
    ElasticWindowMultiple,
    MSDFit,
    BASISReduction,
    BASISPowderDiffraction,
    Load,
    Divide,
    ReplaceSpecialValues,
)


class PreppingMixin(object):
    r"""Common code for tests classes"""

    def prepset(self, subdir):
        self.config = {p: config[p] for p in ("default.facility", "default.instrument", "datasearch.directories")}
        config["default.facility"] = "SNS"
        config["default.instrument"] = "BASIS"
        config.appendDataSearchSubDir("BASIS/{}/".format(subdir))

    def preptear(self):
        for key, value in self.config.items():
            config[key] = value  # config object does not have update method like python dict


class ElwinTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""ELWIN tab of the Indirect Inelastic Interface"""

    def __init__(self):
        super(ElwinTest, self).__init__()
        self.config = None
        self.prepset("ELWIN")

    def requiredFiles(self):
        return ["BASIS_63652_sqw.nxs", "BASIS_63700_sqw.nxs", "BASIS_elwin_eq2.nxs"]

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            # Load files and create workspace group
            names = ("BASIS_63652_sqw", "BASIS_63700_sqw")
            [Load(Filename=name + ".nxs", OutputWorkspace=name) for name in names]
            GroupWorkspaces(InputWorkspaces=names, OutputWorkspace="elwin_input")
            ElasticWindowMultiple(
                InputWorkspaces="elwin_input",
                IntegrationRangeStart=-0.0035,
                IntegrationRangeEnd=0.0035,
                BackgroundRangeStart=-0.1,
                BackgroundRangeEnd=-0.05,
                SampleEnvironmentLogName="SensorA",
                SampleEnvironmentLogValue="average",
                OutputInQ="outQ",
                OutputInQSquared="outQ2",
                OutputELF="ELF",
                OutputELT="ELT",
            )
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "outQ2", "BASIS_elwin_eq2.nxs"


class GaussianMSDTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""MSD tab of the Indirect Inelastic Interface"""

    def __init__(self):
        super(GaussianMSDTest, self).__init__()
        self.config = None
        self.prepset("MSD")

    def requiredFiles(self):
        return ["BASIS_63652_63720_elwin_eq.nxs", "BASIS_63652_63720_Gaussian_msd.nxs"]

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            Load(Filename="BASIS_63652_63720_elwin_eq.nxs", OutputWorkspace="elwin_eq")
            MSDFit(InputWorkspace="elwin_eq", Model="Gauss", SpecMax=68, XStart=0.3, XEnd=1.3, OutputWorkspace="outMSD")
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "outMSD", "BASIS_63652_63720_Gaussian_msd.nxs"


class BASISReduction1Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the old DAS using: (1)silicon 111 analyzers, (2) monitor
    normalization, (3) vanadium normalization"""

    def __init__(self):
        super(BASISReduction1Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_111.xml", "BSS_79827_event.nxs", "BSS_64642_event.nxs", "BASIS_79827_divided_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="79827",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                ReflectionType="silicon_111",
                MaskFile="BASIS_Mask_default_111.xml",
                EnergyBins=[-100, 0.4, 100],
                MomentumTransferBins=[1.1, 1.8, 1.1],
                DivideByVanadium=True,
                NormRunNumbers="64642",
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_79827_divided_sqw", "BASIS_79827_divided_sqw.nxs"


class BASISReduction2Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the old DAS using: (1)silicon 311 analyzers, (2) proton
    chaarge normalization, (3) vanadium normalization"""

    def __init__(self):
        super(BASISReduction2Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_311.xml", "BSS_56748_event.nxs", "BSS_78379_event.nxs", "BASIS_56748_divided_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="56748",
                DoFluxNormalization=True,
                FluxNormalizationType="Proton Charge",
                ReflectionType="silicon_311",
                MaskFile="BASIS_Mask_default_311.xml",
                EnergyBins=[-740.0, 1.6, 740.0],
                MomentumTransferBins=[2.1, 3.4, 2.1],
                DivideByVanadium=True,
                NormRunNumbers="78379",
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_56748_divided_sqw", "BASIS_56748_divided_sqw.nxs"


class BASISReduction3Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the new DAS using: (1)silicon 111 analyzers, (2) monitor
    normalization, (3) vanadium normalization"""

    def __init__(self):
        super(BASISReduction3Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_111.xml", "BSS_90177.nxs.h5", "BSS_90176.nxs.h5", "BASIS_90177_divided_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="90177",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                ReflectionType="silicon_111",
                MaskFile="BASIS_Mask_default_111.xml",
                EnergyBins=[-100, 0.4, 100],
                MomentumTransferBins=[1.1, 1.8, 1.1],
                DivideByVanadium=True,
                NormRunNumbers="90176",
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_90177_divided_sqw", "BASIS_90177_divided_sqw.nxs"


class BASISReduction4Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the new DAS using: (1)silicon 311 analyzers, (2) monitor
    normalization, (3) vanadium normalization"""

    def __init__(self):
        super(BASISReduction4Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_311.xml", "BSS_90178.nxs.h5", "BSS_90176.nxs.h5", "BASIS_90178_divided_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="90178",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                MaskFile="BASIS_Mask_default_311.xml",
                ReflectionType="silicon_311",
                EnergyBins=[-740.0, 1.6, 740.0],
                MomentumTransferBins=[2.1, 3.4, 2.1],
                DivideByVanadium=True,
                NormRunNumbers="90176",
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_90178_divided_sqw", "BASIS_90178_divided_sqw.nxs"


class BASISReduction5Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the new DAS using: (1)silicon 333 analyzers, (2) monitor
    normalization"""

    def __init__(self):
        super(BASISReduction5Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_333.xml", "BSS_90146.nxs.h5", "BASIS_90146_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="90146",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                MaskFile="BASIS_Mask_default_333.xml",
                ReflectionType="silicon_333",
                EnergyBins=[-330, 4.0, 330],
                MomentumTransferBins=[3.05, 4.30, 3.05],
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_90146_sqw", "BASIS_90146_sqw.nxs"


class BASISReduction6Test(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the new DAS using: (1)silicon 333 analyzers, (2) monitor
    normalization, (3) Vanadium normalization"""

    def __init__(self):
        super(BASISReduction6Test, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_333.xml", "BSS_90146.nxs.h5", "BSS_90175.nxs.h5", "BASIS_90146_divided_sqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="90146",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                MaskFile="BASIS_Mask_default_333.xml",
                ReflectionType="silicon_333",
                EnergyBins=[-330, 4.0, 330],
                MomentumTransferBins=[3.05, 4.30, 3.05],
                DivideByVanadium=True,
                NormRunNumbers="90175",
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_90146_divided_sqw", "BASIS_90146_divided_sqw.nxs"


class DynamicSusceptibilityTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduce in the new DAS using: (1)silicon 333 analyzers, (2) monitor
    normalization, (3) Vanadium normalization"""

    def __init__(self):
        super(DynamicSusceptibilityTest, self).__init__()
        self.config = None
        self.prepset("BASISReduction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_333.xml", "BSS_90146.nxs.h5", "BSS_90175.nxs.h5", "BSS_90146_divided_Xqw.nxs"]

    def runTest(self):
        try:
            BASISReduction(
                RunNumbers="90146",
                DoFluxNormalization=True,
                FluxNormalizationType="Monitor",
                MaskFile="BASIS_Mask_default_333.xml",
                ReflectionType="silicon_333",
                EnergyBins=[-330, 4.0, 330],
                MomentumTransferBins=[3.05, 4.30, 3.05],
                DivideByVanadium=True,
                NormRunNumbers="90175",
                OutputSusceptibility=True,
            )
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "BSS_90146_divided_Xqw", "BSS_90146_divided_Xqw.nxs"


class CrystalDiffractionTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduction for a scan of runs probing different orientations of a
    crystal."""

    def __init__(self):
        super(CrystalDiffractionTest, self).__init__()
        self.config = None
        self.prepset("BASISDiffraction")

    def requiredFiles(self):
        return [
            "BASIS_Mask_default_diff.xml",
            "BSS_74799_event.nxs",
            "BSS_74800_event.nxs",
            "BSS_64642_event.nxs",
            "BSS_75527_event.nxs",
            "BASISOrientedSample.nxs",
        ]

    def runTest(self):
        try:
            BASISCrystalDiffraction(
                RunNumbers="74799-74800",
                MaskFile="BASIS_Mask_default_diff.xml",
                VanadiumRuns="64642",
                BackgroundRuns="75527",
                PsiAngleLog="SE50Rot",
                PsiOffset=-27.0,
                LatticeSizes=[10.71, 10.71, 10.71],
                LatticeAngles=[90.0, 90.0, 90.0],
                VectorU=[1, 1, 0],
                VectorV=[0, 0, 1],
                Uproj=[1, 1, 0],
                Vproj=[0, 0, 1],
                Wproj=[1, -1, 0],
                Nbins=300,
                OutputWorkspace="peaky",
            )
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "peaky", "BASISOrientedSample.nxs"


class PowderSampleTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Run a elastic reduction for powder sample"""

    def __init__(self):
        super(PowderSampleTest, self).__init__()
        self.config = None
        self.prepset("BASISDiffraction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_diff.xml", "BSS_74799_event.nxs", "BSS_75527_event.nxs", "BSS_64642_event.nxs", "BASISPowderSample.nxs"]

    def runTest(self):
        r"""
        Override parent method, does the work of running the test
        """
        try:
            # run with old DAS
            BASISPowderDiffraction(
                RunNumbers="74799",
                OutputWorkspace="powder",
                MaskFile="BASIS_Mask_default_diff.xml",
                BackgroundRuns="75527",
                VanadiumRuns="64642",
            )
        finally:
            self.preptear()

    def validate(self):
        r"""
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "powder", "BASISPowderSample.nxs"


class PowderFluxNormalizationTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Run a elastic reduction for powder sample with two flux
    normalizations"""

    def __init__(self):
        super(PowderFluxNormalizationTest, self).__init__()
        self.config = None
        self.prepset("BASISDiffraction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_diff.xml", "BSS_74799_event.nxs", "BASISPowderFluxNorm.nxs"]

    def runTest(self):
        try:
            BASISPowderDiffraction(
                RunNumbers="74799", FluxNormalizationType="Monitor", OutputWorkspace="powder_Mon", MaskFile="BASIS_Mask_default_diff.xml"
            )
            BASISPowderDiffraction(
                RunNumbers="74799",
                FluxNormalizationType="Proton Charge",
                OutputWorkspace="powder_Pro",
                MaskFile="BASIS_Mask_default_diff.xml",
            )
            Divide(LHSWorkspace="powder_Pro", RHSWorkspace="powder_Mon", OutputWorkspace="powder_ratio")
            ReplaceSpecialValues(InputWorkspace="powder_ratio", NANValue=1.0, NANError=1.0, OutputWorkspace="powder_ratio")
        finally:
            self.preptear()

    def validate(self):
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "powder_ratio", "BASISPowderFluxNorm.nxs"


class PowderSampleNewDASTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Run a elastic reduction for powder sample in the newer DAS"""

    def __init__(self):
        super(PowderSampleNewDASTest, self).__init__()
        self.config = None
        self.prepset("BASISDiffraction")

    def requiredFiles(self):
        return ["BASIS_Mask_default_diff.xml", "BSS_90176.nxs.h5", "BSS_90177.nxs.h5", "BASIS_powder_90177.nxs"]

    def runTest(self):
        r"""
        Override parent method, does the work of running the test
        """
        try:
            # run with new DAS
            BASISPowderDiffraction(
                RunNumbers="90177", OutputWorkspace="powder", MaskFile="BASIS_Mask_default_diff.xml", VanadiumRuns="90176"
            )
        finally:
            self.preptear()

    def validate(self):
        r"""
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(["SpectraMap", "Instrument"])
        return "powder", "BASIS_powder_90177.nxs"
