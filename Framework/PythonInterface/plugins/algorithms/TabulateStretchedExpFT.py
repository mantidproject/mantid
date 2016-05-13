#pylint: disable=no-init
from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import CreateWorkspace, GroupWorkspaces
import numpy as np
import scipy.constants
import operator
import h5py
from pdb import set_trace as tr

"""
@author Jose Borreguero, NScD
@date May 06 2016

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
"""

DEFAULT_BETA_VALUES = [0.2, 0.025, 3.0]
DEFAULT_EVALN = 1000
DEFAULT_TAUMAX = 10000.0
DEFAULT_EMAX = 1.0
PLANCK_CONSTANT = scipy.constants.Planck/scipy.constants.e*1E15  # meV*psec

#pylint: disable=too-many-instance-attributes
class TabulateStretchedExpFT(PythonAlgorithm):
    """Create tables for the Fourier transform of the
    Stretched exponential function.
    """

    def category(self):
        return "Optimization"

    def name(self):
        return "TabulateStretchedExpFT"

    def version(self):
        return 1

    def summary(self):
        return "Create tables for the Fourier transform of the Stretched exponential function."

    def eval_func(self, reduced_energy, beta):
        """Calculate the Fourier transform of the stretched exponential
        Formula:
            y(t)=(1/hbar)*int_0^{\infty} cos(reduced_energy*t) e^{-t^\beta}
        :param reduced_energy: unit-less energy (E*\tau)/\hbar
        :param beta: stretching exponent (beta=1 for pure exponential)
        :return: value of the function
        """
        decay = 1E-06
        tmax = (-np.log(decay))**(1./beta)
        nt = 1E7  # number of points where to evaluate the integral
        dt = tmax/nt
        # dt is our minimal interval, but if cos(reduced_energy*t)
        # changes sign with high frequency in the interval dt, we
        # choose a smaller dt, with 2pi = reduced_energy * t * 2**m.
        # This ensures that cos(reduced_energy*t) is positive
        # and negative exactly 2**(m-1) times, cancelling each other.
        # If we don't do this, then we are "randomly" sampling
        # cos(reduced_energy*t), and the integral could be negative
        # if we sampled negative cos(reduced_energy*t) values more
        # than the positive ones.
        if reduced_energy > 0:
            m = 5  # 2**m is number of evaluations in a 2pi interval
            ds = 2*np.pi/(reduced_energy*(2**m))
            if ds < dt:
                dt = ds
                tmax = dt * nt
        t = np.arange(0, tmax, dt)
        z = t**beta
        y = dt * np.cos(reduced_energy*t)*np.exp(-z)
        return y.sum()

    def split_bin(self, max_bin, beta):
        """Split a bin "object" into two.
        Recall the function decreases with increasing reduced_energy
        :param max_bin:
        :return: dictionary with the two split objects
        """
        umin, ymax = max_bin[0]
        umax, ymin = max_bin[1]
        umiddle = (umin+umax)/2.0
        ymiddle = self.eval_func(umiddle, beta)
        left_bin = [(umin,ymax), (umiddle,ymiddle)]
        right_bin = [(umiddle,ymiddle), (umax,ymin)]
        return {abs(ymax-ymiddle): left_bin, abs(ymiddle-ymin): right_bin}

    def getPoints(self, bins):
        """Scan the bins and return a set of points (u,y)
        ordered by increasing u-value
        :param bins: dictionary of bins step:[(umin,ymax),(umax,ymin)]
        :return: ordered list of pairs (u,y) by increasing u-value
        """
        boundaries = list()
        for bin in bins.values():
            left_boundary = bin[0] # (umin, ymax)
            boundaries.append(left_boundary)
        # sort boundaries by increasing u-value
        boundaries.sort(key=operator.itemgetter(0))
        return boundaries

    def tabulate(self, beta):
        """Carry out the tables calculation
        :param beta: stretching exponent
        :return: arrays of reduced_values and function_values
        """
        umax = self._emax*2*np.pi*self._taumax/PLANCK_CONSTANT  # max reduced energy
        # Calculate (u,y) end points for the tables at the given beta
        ymax = self.eval_func(0.0, beta)
        ymin = self.eval_func(umax, beta)
        dy = ymax-ymin
        initial_bin = [(0.0,ymax), (umax,ymin)]

        # Find (u,y) points that produce a regular grid of evalN points
        # on the y-axis
        bins = {dy:initial_bin,}
        i = 2 # number of evaluated points
        while i<self._evalN:
            max_decrease = max(bins.keys()) # find bind where signal decreases most
            # Split the bin in two
            two_bins = self.split_bin(bins[max_decrease], beta)
            del bins[max_decrease]
            bins.update(two_bins)
            i += 1
        # Get list of (u,y) pairs
        points = self.getPoints(bins)
        points.append((umax, ymin))  # the list miss the minimal value
        return [np.array(x) for x in zip(*points)]

    def SaveToFile(self):
        """Save the following tables to HDF5 file as datasets:
        1. Reduced energies ('reduced_energies')
        2. Values of the function ('tables')
        3. Values of the stretching exponent ('betas')
        """
        # Validate the number of workspaces is same as number of stretching exponents
        nBetas = self._workspace_group.getNumberOfEntries()
        if nBetas != len(self._betas):
            raise ValueError("WorkspaceGroup contains incorrect number of tables")
        # Fill the tables using the workspaces
        arrays = {'betas': self._betas,
                  'reduced_energies': np.empty((nBetas,self._evalN)),
                  'tables': np.empty((nBetas,self._evalN))}
        for iBeta in range(nBetas):
            workspace = self._workspace_group.getItem(iBeta)
            arrays['reduced_energies'][iBeta] = workspace.dataX(0)
            arrays['tables'][iBeta] = workspace.dataY(0)
        # Save to file
        file_handle = h5py.File(self._tablesFile, "w")
        for name,value in arrays.items():
            dset = file_handle.create_dataset(name, value.shape, dtype='f')
            dset[...] = value
        file_handle.close()

    def PyInit(self):
        betas_bounds_validator = FloatArrayBoundedValidator()
        betas_bounds_validator.setLower(0.0)
        betas_bounds_validator.setUpper(5.0)
        self.declareProperty(FloatArrayProperty("BetaValues",
                                                DEFAULT_BETA_VALUES,
                                                validator=betas_bounds_validator,
                                                direction=Direction.Input),
                             doc="list of beta values (min, delta, max)")
        evalN_validator = IntBoundedValidator()
        evalN_validator.setBounds(2, 10000)
        self.declareProperty("EvalN", DEFAULT_EVALN,
                             evalN_validator,
                             direction=Direction.Input,
                             doc="number of points to evaluate the function")
        taumax_validator = FloatBoundedValidator()
        taumax_validator.setBounds(0.0, 100000.0)  # zero to 100ns
        self.declareProperty("Taumax", DEFAULT_TAUMAX,
                             taumax_validator,
                             direction=Direction.Input,
                             doc="maximum relaxation time, in psec.")
        emax_validator = FloatBoundedValidator()
        emax_validator.setBounds(0.0, 10.0)  # zero to 10meV
        self.declareProperty("Emax", DEFAULT_EMAX,
                             emax_validator,
                             direction=Direction.Input,
                             doc="maximum energy, in meV.")
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace",
                                               defaultValue="",
                                               direction=Direction.Output,
                                               optional=PropertyMode.Optional),
                             doc="Save tables to WorkspaceGroup")
        self.declareProperty(FileProperty("TablesFileName","",
                                          FileAction.OptionalSave, ['.h5']),
                             doc="Name of the output HDF5 tables file.")

    def PyExec(self):
        self._evalN = self.getProperty("EvalN").value
        self._taumax = self.getProperty("Taumax").value
        self._emax = self.getProperty("Emax").value
        outputWorkspace_name = self.getProperty("OutputWorkspace").valueAsStr
        if not outputWorkspace_name:
            outputWorkspace_name = "__tables_stretchedExFT"
        betaMin, deltaBeta, betaMax = self.getProperty("BetaValues").value
        self._workspace_group = None
        self._betas = np.arange(betaMin, betaMax, deltaBeta)
        for beta in self._betas:
            reduced_energies, function_values = self.tabulate(beta)
            workspace_name = "tables_{0:06.3f}".format(beta)
            CreateWorkspace(reduced_energies, function_values,
                            OutputWorkspace=workspace_name)
            if not self._workspace_group:
                # instantiate the workspace group
                self._workspace_group = GroupWorkspaces(workspace_name,
                                                        OutputWorkspace=outputWorkspace_name)
            else:
                self._workspace_group.add(workspace_name)
        self.setProperty("OutputWorkspace", self._workspace_group)
        # Save tables to file, if so required
        self._tablesFile = self.getProperty("TablesFileName").value
        if self._tablesFile:
            self.SaveToFile()
        #return self._workspace_group

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(TabulateStretchedExpFT)
