# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import systemtesting
from mantid import config
from mantid.simpleapi import (BASISCrystalDiffraction, Load, GroupWorkspaces,
                              ElasticWindowMultiple, MSDFit,
                              BASISPowderDiffraction)


class PreppingMixin(object):
    r"""Common code for tests classes
    """
    def prepset(self, subdir):
        self.config = {p: config[p] for p in ('default.facility',
                                              'default.instrument',
                                              'datasearch.directories')}
        config['default.facility'] = 'SNS'
        config['default.instrument'] = 'BASIS'
        config.appendDataSearchSubDir('BASIS/{}/'.format(subdir))

    def preptear(self):
        for (key, value) in self.config.items():
            config[key] = value  # config object does not have update method like python dict


class ElwinTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""ELWIN tab of the Indirect Inelastic Interface
    """

    def __init__(self):
        super(ElwinTest, self).__init__()
        self.config = None
        self.prepset('ELWIN')

    def requiredFiles(self):
        return ['BASIS_63652_sqw.nxs',
                'BASIS_63700_sqw.nxs',
                'BASIS_elwin_eq2.nxs']

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            # Load files and create workspace group
            names = ('BASIS_63652_sqw', 'BASIS_63700_sqw')
            [Load(Filename=name + '.nxs', OutputWorkspace=name) for name in names]
            GroupWorkspaces(InputWorkspaces=names, OutputWorkspace='elwin_input')
            ElasticWindowMultiple(InputWorkspaces='elwin_input',
                                  IntegrationRangeStart=-0.0035,
                                  IntegrationRangeEnd=0.0035,
                                  BackgroundRangeStart=-0.1,
                                  BackgroundRangeEnd=-0.05,
                                  SampleEnvironmentLogName='SensorA',
                                  SampleEnvironmentLogValue='average',
                                  OutputInQ='outQ',
                                  OutputInQSquared='outQ2',
                                  OutputELF='ELF',
                                  OutputELT='ELT')
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(['SpectraMap', 'Instrument'])
        return 'outQ2', 'BASIS_elwin_eq2.nxs'


class GaussianMSDTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""MSD tab of the Indirect Inelastic Interface
    """

    def __init__(self):
        super(GaussianMSDTest, self).__init__()
        self.config = None
        self.prepset('MSD')

    def requiredFiles(self):
        return ['BASIS_63652_63720_elwin_eq.nxs',
                'BASIS_63652_63720_Gaussian_msd.nxs']

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            Load(Filename='BASIS_63652_63720_elwin_eq.nxs',
                 OutputWorkspace='elwin_eq')
            MSDFit(InputWorkspace='elwin_eq', Model='Gauss', SpecMax=68,
                   XStart=0.3, XEnd=1.3, OutputWorkspace='outMSD')
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(['SpectraMap', 'Instrument'])
        return 'outMSD', 'BASIS_63652_63720_Gaussian_msd.nxs'


class CrystalDiffractionTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Reduction for a scan of runs probing different orientations of a crystal.
    """

    def __init__(self):
        super(CrystalDiffractionTest, self).__init__()
        self.config = None
        self.prepset('BASISCrystalDiffraction')

    def requiredFiles(self):
        return ['BASIS_Mask_default_diff.xml',
                'BSS_74799_event.nxs',
                'BSS_74800_event.nxs',
                'BSS_64642_event.nxs',
                'BSS_75527_event.nxs',
                'BASISOrientedSample.nxs']

    def runTest(self):
        """
        Override parent method, does the work of running the test
        """
        try:
            BASISCrystalDiffraction(RunNumbers='74799-74800',
                                    MaskFile='BASIS_Mask_default_diff.xml',
                                    VanadiumRuns='64642',
                                    BackgroundRuns='75527',
                                    PsiAngleLog='SE50Rot',
                                    PsiOffset=-27.0,
                                    LatticeSizes=[10.71, 10.71, 10.71],
                                    LatticeAngles=[90.0, 90.0, 90.0],
                                    VectorU=[1, 1, 0],
                                    VectorV=[0, 0, 1],
                                    Uproj=[1, 1, 0],
                                    Vproj=[0, 0, 1],
                                    Wproj=[1, -1, 0],
                                    Nbins=300,
                                    OutputWorkspace='peaky')
        finally:
            self.preptear()

    def validate(self):
        """
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(['SpectraMap', 'Instrument'])
        return 'peaky', 'BASISOrientedSample.nxs'


class PowderSampleTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Run a elastic reduction for powder sample"""

    def __init__(self):
        super(PowderSampleTest, self).__init__()
        self.config = None
        self.prepset('BASISDiffraction')

    def requiredFiles(self):
        return ['BASIS_Mask_default_diff.xml',
                'BSS_74799_event.nxs', 'BSS_75527_event.nxs',
                'BSS_64642_event.nxs', 'BASISPowderSample.nxs']

    def runTest(self):
        r"""
        Override parent method, does the work of running the test
        """
        try:
            # run with old DAS
            BASISPowderDiffraction(RunNumbers='74799',
                                   OutputWorkspace='powder',
                                   BackgroundRuns='75527',
                                   VanadiumRuns='64642')
        finally:
            self.preptear()

    def validate(self):
        r"""
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(['SpectraMap', 'Instrument'])
        return 'powder', 'BASISPowderSample.nxs'


class PowderSampleNewDASTest(systemtesting.MantidSystemTest, PreppingMixin):
    r"""Run a elastic reduction for powder sample in the newer DAS"""

    def __init__(self):
        super(PowderSampleNewDASTest, self).__init__()
        self.config = None
        self.prepset('BASISDiffraction')

    def requiredFiles(self):
        return ['BASIS_Mask_default_diff.xml',
                'BSS_90176.nxs.h5', 'BSS_90177.nxs.h5',
                'BASIS_powder_90177.nxs']

    def runTest(self):
        r"""
        Override parent method, does the work of running the test
        """
        try:
            # run with new DAS
            BASISPowderDiffraction(RunNumbers='90177',
                                   OutputWorkspace='powder',
                                   VanadiumRuns='90176')
        finally:
            self.preptear()

    def validate(self):
        r"""
        Inform of workspace output after runTest(), and associated file to
        compare to.
        :return: strings for workspace and file name
        """
        self.tolerance = 0.1
        self.disableChecking.extend(['SpectraMap', 'Instrument'])
        return 'powder', 'BASIS_powder_90177.nxs'
