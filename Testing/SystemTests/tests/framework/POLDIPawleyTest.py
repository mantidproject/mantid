# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-few-public-methods
import systemtesting
import numpy as np
from mantid.api import AnalysisDataService as ADS, FileFinder
from mantid.simpleapi import PoldiAutoCorrelation, ConvertUnits, ConvertToPointData, ExtractSpectra, FlatBackground
from Engineering.pawley_utils import Phase, GaussianProfile, PawleyPattern1D, PawleyPattern2D
from plugins.algorithms.poldi_utils import load_poldi_h5f


class POLDIPawleyWorkflow(systemtesting.MantidSystemTest):
    def setUp(self):
        fpath_data = FileFinder.getFullPath("poldi2025n012174.hdf")  # for np.loadtxt so need full path
        self.ws = load_poldi_h5f(fpath_data, "POLDI_Definition_896.xml", t0=5.824e-02, t0_const=-12.68, output_workspace="poldi_silicon")
        self.ws_autocorr = PoldiAutoCorrelation(self.ws, OutputWorkspace=self.ws.name() + "_ac", NGroups=4)
        self.ws_autocorr = ConvertUnits(InputWorkspace=self.ws_autocorr, OutputWorkspace=self.ws_autocorr.name(), Target="dSpacing")
        self.ws_autocorr = ConvertToPointData(InputWorkspace=self.ws_autocorr, OutputWorkspace=self.ws_autocorr.name())
        self.ispec = 2  #  North Bank (low tth half of bank)
        self.ws_crop = ExtractSpectra(
            InputWorkspace=self.ws,
            OutputWorkspace=f"{self.ws.name()}_crop",
            DetectorList=self.ws_autocorr.getSpectrum(self.ispec).getDetectorIDs(),
        )

    def cleanup(self):
        ADS.clear()

    def runTest(self):
        # setup phase
        si = Phase.from_alatt(3 * [5.43105], "F d -3 m", "Si 3/4 3/4 1/4 1.0 0.08")
        si.set_hkls_from_dspac_limits(0.7, 3.5)
        si.hkls = [hkl for ihkl, hkl in enumerate(si.hkls) if ihkl not in (3, 11, 14)]  # remove absent peaks
        si.merge_reflections()

        # run 1D Pawley refinement
        pawley1d = PawleyPattern1D(self.ws_autocorr, [si], profile=GaussianProfile(), bg_func=FlatBackground(), ispec=self.ispec)
        pawley1d.estimate_initial_params()
        res = pawley1d.fit()
        # collect attributes to assert
        self.cost_1d = res.cost
        self.alatt_1d = pawley1d.alatt_params[0][0]
        self.alatt_1d_err = pawley1d.get_parameter_errors(res)[0]

        # 2D Pawley refinement
        pawley2d = PawleyPattern2D(self.ws_crop, [si], profile=GaussianProfile(), global_scale=False)
        pawley2d.set_params_from_pawley1d(pawley1d)
        res = pawley2d.fit()
        # collect attributes to assert
        self.cost_2d = res.cost
        self.alatt_2d = pawley1d.alatt_params[0][0]
        ws2d_sim = ADS.retrieve(f"{self.ws_crop.name()}_sim")
        self.ws2d_sim_nspec = ws2d_sim.getNumberHistograms()
        self.ws2d_sim_detid = ws2d_sim.getSpectrum(0).getDetectorIDs()[0]

        # Do unconstrained 2D fit (all peaks independent)
        pawley2d_free = pawley2d.create_no_constriants_fit()
        res = pawley2d_free.fit()
        # collect attributes to assert
        self.cost_2d_free = res.cost
        # get residual d-spacing
        dspacs_free = pawley2d_free.get_peak_centres()
        dspacs_pawley = pawley2d.phases[0].calc_dspacings()
        self.residual_dspacs = dspacs_pawley - dspacs_free

    def validate(self):
        # 1D Pawley
        self.assertAlmostEqual(self.alatt_1d, 5.43122, delta=1e-4)
        self.assertAlmostEqual(self.cost_1d, 206.9, delta=1e-1)
        self.assertAlmostEqual(self.alatt_1d_err, 1.1e-4, delta=1e-5)

        # 2D Pawley
        self.assertAlmostEqual(self.alatt_2d, 5.43122, delta=1e-4)
        self.assertAlmostEqual(self.cost_2d, 248117, delta=1)
        self.assertEqual(self.ws2d_sim_nspec, 224)
        self.assertEqual(self.ws2d_sim_detid, 211001)

        # 2D Pawley Free
        self.assertLessThan(self.cost_2d_free, self.cost_2d)
        self.assertLessThan(np.mean(abs(self.residual_dspacs)), 2e-04)
        return True
