# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

from mantid.api import (PythonAlgorithm, AlgorithmFactory,
                        MatrixWorkspaceProperty, Progress, InstrumentValidator,
                        ITableWorkspaceProperty)
from mantid.kernel import Direction, FloatBoundedValidator, Property
import numpy as np
from scipy import integrate
import scipy as sp


class ComputeCalibrationCoefVan(PythonAlgorithm):
    """ Calculate coefficients to normalize by Vanadium and correct Debye
        Waller factor
    """

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.vanaws = None
        self.defaultT = 293.0       # K, default temperature if not given
        self.Mvan = 50.942          # [g/mol], Vanadium molar mass
        self.DebyeT = 389.0         # K, Debye temperature for Vanadium

    def category(self):
        """ Return category
        """
        return "CorrectionFunctions\\EfficiencyCorrections"

    def name(self):
        """ Return summary
        """
        return "ComputeCalibrationCoefVan"

    def summary(self):
        return ("Calculate coefficients for detector efficiency correction " +
                "using the Vanadium data.")

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty(
                             "VanadiumWorkspace", "",
                             direction=Direction.Input,
                             validator=InstrumentValidator()),
                             "Input Vanadium workspace")
        self.declareProperty(ITableWorkspaceProperty("EPPTable", "",
                             direction=Direction.Input),
                             ("Input EPP table. May be produced by FindEPP " +
                              "algorithm."))
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "",
                             direction=Direction.Output),
                             ("Name the workspace that will contain the " +
                              "calibration coefficients"))
        self.declareProperty("Temperature",
                             defaultValue=Property.EMPTY_DBL,
                             validator=FloatBoundedValidator(lower=0.0),
                             direction=Direction.Input,
                             doc=("Temperature during the experiment (in " +
                                  "Kelvins) if the 'temperature' sample log " +
                                  "is missing or needs to be overriden."))
        return

    def validateInputs(self):
        """ Validate the inputs
        """
        issues = dict()
        inws = self.getProperty("VanadiumWorkspace").value
        run = inws.getRun()

        if not run.hasProperty('wavelength'):
            issues['VanadiumWorkspace'] = ("Input workspace must have " +
                                           "wavelength sample log.")
        else:
            try:
                float(run.getProperty('wavelength').value)
            except ValueError:
                issues['VanadiumWorkspace'] = ("Invalid value for " +
                                               "wavelength sample log. " +
                                               "Wavelength must be a number.")

        table = self.getProperty("EPPTable").value
        if table.rowCount() != inws.getNumberHistograms():
            issues['EPPTable'] = ("Number of rows in the table must match " +
                                  "to the input workspace dimension.")
        # table must have 'PeakCentre' and 'Sigma' columns
        if 'PeakCentre' not in table.getColumnNames():
            issues['EPPTable'] = "EPP Table must have the PeakCentre column."
        if 'Sigma' not in table.getColumnNames():
            issues['EPPTable'] = "EPP Table must have the Sigma column."

        return issues

    def get_temperature(self):
        """Return the temperature
        """
        if not self.getProperty("Temperature").isDefault:
            return self.getProperty("Temperature").value
        run = self.vanaws.getRun()
        if not run.hasProperty('temperature'):
            self.log().warning("No Temperature given and the 'temperature' " +
                               "sample log is not present in " +
                               self.vanaws.name() +
                               " T=293K is assumed for Debye-Waller factor.")
            return self.defaultT
        try:
            temperature = float(run.getProperty('temperature').value)
        except ValueError as err:
            self.log().warning("Error of getting temperature from the " +
                               "sample log " + err + " T=293K is assumed " +
                               "for Debye-Waller factor.")
            return self.defaultT

        return temperature

    def PyExec(self):
        """ Main execution body
        """

        # returns workspace instance
        self.vanaws = self.getProperty("VanadiumWorkspace").value
        # returns workspace name (string)
        eppws = self.getProperty("EPPTable").value
        nhist = self.vanaws.getNumberHistograms()
        prog_reporter = Progress(self, start=0.8, end=1.0, nreports=3)
        integrate = self.createChildAlgorithm("IntegrateEPP", startProgress=0.0, endProgress=0.8, enableLogging=False)
        integrate.setProperty("InputWorkspace", self.vanaws)
        integrate.setProperty("OutputWorkspace", "__unused_for_child")
        integrate.setProperty("EPPWorkspace", eppws)
        width = 3. * 2. * np.sqrt(2. * np.log(2.))
        integrate.setProperty("HalfWidthInSigmas", width)
        integrate.execute()
        prog_reporter.report("Computing DWFs")
        outws = integrate.getProperty("OutputWorkspace").value
        # calculate array of Debye-Waller factors
        prog_reporter.report("Applying DWFs")
        dwf = self.calculate_dwf()
        for idx in range(nhist):
            ys = outws.dataY(idx)
            ys /= dwf[idx]
            es = outws.dataE(idx)
            es /= dwf[idx]
        prog_reporter.report("Done")
        self.setProperty("OutputWorkspace", outws)

    def calculate_dwf(self):
        """
        Calculates Debye-Waller factor according to
        Sears and Shelley Acta Cryst. A 47, 441 (1991)
        """
        run = self.vanaws.getRun()
        nhist = self.vanaws.getNumberHistograms()
        thetasort = np.empty(nhist)  # half of the scattering angle, in radians
        for i in range(nhist):
            det = self.vanaws.getDetector(i)
            thetasort[i] = 0.5 * self.vanaws.detectorTwoTheta(det)

        # T in K
        temperature = self.get_temperature()
        # Wavelength, Angstrom
        wlength = float(run.getLogData('wavelength').value)
        # Vanadium mass, kg
        mass_vana = 0.001*self.Mvan/sp.constants.N_A
        temp_ratio = temperature/self.DebyeT

        if temp_ratio < 1.e-3:
            integral = 0.5
        else:
            integral = \
                integrate.quad(lambda x: x/sp.tanh(0.5*x/temp_ratio), 0, 1)[0]

        msd = 3.*sp.constants.hbar**2 / \
            (2.*mass_vana*sp.constants.k * self.DebyeT)*integral*1.e20
        return np.exp(-msd*(4.*sp.pi*sp.sin(thetasort)/wlength)**2)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(ComputeCalibrationCoefVan)
