# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
#
import unittest

from mantid.api import AnalysisDataService
from mantid.simpleapi import CreateSampleWorkspace, SetSampleMaterial
from mantidqt.utils.qt.testing import start_qapplication

from mantidqt.widgets.samplematerialdialog.samplematerial_presenter import SampleMaterialDialogPresenter


@start_qapplication
class SampleMaterialPresenterTest(unittest.TestCase):

    def setUp(self):
        # Create a workspace and change the sample material to non-trivial values.
        workspace_name = "testWS"
        CreateSampleWorkspace(OutputWorkspace=workspace_name)
        self.ws = AnalysisDataService.retrieve(workspace_name)
        self.formula = "C"
        self.number_density = 3.4e21
        self.absorption_xs = 3.5
        self.scattering_xs = 6.7
        self.coherent_xs = 3.2
        self.incoherent_xs = 3.4

        SetSampleMaterial(InputWorkspace=self.ws,
                          ChemicalFormula=self.formula,
                          SampleNumberDensity=self.number_density,
                          AttenuationXSection=self.absorption_xs,
                          ScatteringXSection=self.scattering_xs,
                          CoherentXSection=self.coherent_xs,
                          IncoherentXSection=self.incoherent_xs)

        self.presenter = SampleMaterialDialogPresenter(self.ws)

    def tearDown(self):
        AnalysisDataService.clear()

    def test_sample_material_update(self):
        """
        Check that the sample material values in the view are up to date.
        The view should have been updated when the presenter was created.
        """
        self.assertEqual(self.presenter.view.formula_item.text(1), self.formula)
        self.assertEqual(self.presenter.view.number_density_item.text(1), str(self.number_density))
        self.assertEqual(self.presenter.view.absorption_item.text(1), str(self.absorption_xs))
        self.assertEqual(self.presenter.view.total_item.text(1), str(self.scattering_xs))
        self.assertEqual(self.presenter.view.coherent_item.text(1), str(self.coherent_xs))
        self.assertEqual(self.presenter.view.incoherent_item.text(1), str(self.incoherent_xs))


if __name__ == '__main__':
    unittest.main()
