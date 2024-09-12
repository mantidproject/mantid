# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# Author: Alex Phimister 08/2016
# pylint: disable=invalid-name
import math
import numpy as np
import scipy as sc

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *


class NMoldyn4Interpolation(PythonAlgorithm):
    def category(self):
        return "Simulation; Inelastic\\DataHandling"

    def summary(self):
        return "Maps NMoldyn simulated s(q,e) data onto OSIRIS' Q and E values"

    def PyInit(self):
        self.declareProperty(
            WorkspaceProperty(name="InputWorkspace", defaultValue="", direction=Direction.Input), doc="Simulated workspace"
        )

        self.declareProperty(
            WorkspaceProperty(name="ReferenceWorkspace", defaultValue="", direction=Direction.Input),
            doc="Reference OSIRIS workspace to provide values",
        )

        self.declareProperty(
            name="EFixed",
            defaultValue=1.845,
            doc=("EFixed value of OSIRIS data (should be default" " in almost all circumstances)"),
            validator=FloatMandatoryValidator(),
            direction=Direction.Input,
        )

        self.declareProperty(
            WorkspaceProperty(name="OutputWorkspace", defaultValue="", direction=Direction.Output),
            doc="Output Workspace of remapped simulation data",
        )

    def PyExec(self):
        e_fixed = float(self.getPropertyValue("EFixed"))
        # Loads simulated workspace
        simulation = self.getPropertyValue("InputWorkspace")
        sim_data = self.load_simulated_dataset(simulation)
        sim_X = sim_data[0]
        sim_Q = sim_data[1]
        sim_Y = sim_data[2]

        # Loads reference workspace
        reference = self.getPropertyValue("ReferenceWorkspace")
        ref_data = self.load_OSIRIS_inst_values(reference, e_fixed)
        ref_X = ref_data[0]
        ref_Q = ref_data[1]
        ref_X_bins = ref_data[2]

        self.validate_bounds(sim_X, ref_X, sim_Q, ref_Q)

        # Interpolates the simulated data onto the OSIRIS grid

        interp = sc.interpolate.RectBivariateSpline(sim_Q, sim_X, sim_Y)
        interp_Y = interp(ref_Q, ref_X_bins)

        # Outputs interpolated data into a new workspace
        interp_Y = interp_Y.flatten()
        ref_X = np.tile(ref_X, len(ref_Q))
        output_ws = self.getPropertyValue("OutputWorkspace")
        CreateWorkspace(
            OutputWorkspace=output_ws,
            DataX=ref_X,
            DataY=interp_Y,
            NSpec=len(ref_Q),
            VerticalAxisUnit="MomentumTransfer",
            VerticalAxisValues=ref_Q,
            WorkspaceTitle=output_ws,
        )
        self.setProperty("OutputWorkspace", output_ws)

    def validate_bounds(self, sim_X, ref_X, sim_Q, ref_Q):
        if min(sim_X) > min(ref_X):
            raise ValueError("Minimum simulated X value is higher than minimum " "reference X value")
        if max(sim_X) < max(ref_X):
            raise ValueError("Maximum simulated X value is lower than maximum " "reference X value")
        if min(sim_Q) > min(ref_Q):
            raise ValueError("Minimum simulated Q value is higher than minimum " "reference Q value")
        if max(sim_Q) < max(ref_Q):
            raise ValueError("Maximum simulated Q value is lower than maximum " "reference Q value")
        else:
            return

    def get_Q_for_workspace_index(self, workspace_name, workspace_index, e_fixed):
        # Calculates Q-values from detectors metadata
        # Inputs are name of workspace, index of detector and fixed energy value
        # Outputs a float Q-value for given ws and detector
        ws = mtd[workspace_name]

        det = ws.getDetector(workspace_index)
        two_theta = ws.detectorTwoTheta(det) / 2
        sin_theta = math.sin(two_theta)
        numer = 1 / sc.constants.angstrom * sc.constants.Planck
        denom = math.sqrt(2 * sc.constants.neutron_mass * sc.constants.eV * 1e-3)
        factor = numer / denom
        power = -0.5
        wavelength = factor * math.pow(e_fixed, power)
        Q = (4.0 * math.pi * sin_theta) / wavelength
        return Q

    def load_OSIRIS_inst_values(self, ws_name, e_fixed):
        # Loads the data from the simulated workspace
        # Note that the two workspaces are slightly different formats, hence two methods
        # Inputs the name of the workspace to load
        # Outputs a tuple of (array of X-values, array of Q-values,
        # list of Y-values, list of X-bins)
        osiris = mtd[ws_name]
        Q_values = []
        for i in range(osiris.getNumberHistograms()):
            Q_values.append(self.get_Q_for_workspace_index(osiris.name(), i, e_fixed))
        X_values = osiris.readX(0)
        X_diff = np.diff(X_values) / 2
        X_bins = [X_values[i] + X_diff[i] for i in range(len(X_diff))]
        return X_values, Q_values, X_bins

    def load_simulated_dataset(self, ws_name):
        # Loads the data from the simulated workspace
        # Inputs the name of the workspace to load
        # Outputs a tuple of (array of X-values, array of Q-values,
        # list of Y-values)
        ws_handle = mtd[ws_name]
        Q_values = ws_handle.getAxis(1).extractValues()
        E_values = ws_handle.readX(0)
        Y_values = [ws_handle.readY(i) for i in range(ws_handle.getNumberHistograms())]
        return E_values, Q_values, Y_values


AlgorithmFactory.subscribe(NMoldyn4Interpolation)
