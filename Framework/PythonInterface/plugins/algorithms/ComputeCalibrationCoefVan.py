# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress, InstrumentValidator, ITableWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, FloatTimeSeriesProperty, Property
import numpy as np
from scipy import integrate
import scipy as sp


class ComputeCalibrationCoefVan(PythonAlgorithm):
    """Calculate coefficients to normalize by Vanadium and correct Debye
    Waller factor
    """

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)
        self.vanaws = None
        self.defaultT = 293.0  # K, default temperature if not given
        self.Mvan = 50.942  # [g/mol], Vanadium molar mass
        self.DebyeT = 389.0  # K, Debye temperature for Vanadium

    def category(self):
        """Return category."""
        return "CorrectionFunctions\\EfficiencyCorrections"

    def name(self):
        """Return algorithm's name."""
        return "ComputeCalibrationCoefVan"

    def summary(self):
        """Return summary."""
        return "Calculate coefficients for detector efficiency correction " + "using the Vanadium data."

    def PyInit(self):
        """Declare properties."""
        self.declareProperty(
            MatrixWorkspaceProperty("VanadiumWorkspace", "", direction=Direction.Input, validator=InstrumentValidator()),
            "Input Vanadium workspace",
        )
        self.declareProperty(
            ITableWorkspaceProperty("EPPTable", "", direction=Direction.Input),
            ("Input EPP table. May be produced by FindEPP " + "algorithm."),
        )
        self.declareProperty(
            MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
            ("Name the workspace that will contain the " + "calibration coefficients"),
        )
        self.declareProperty(
            "Temperature",
            defaultValue=Property.EMPTY_DBL,
            validator=FloatBoundedValidator(lower=0.0),
            direction=Direction.Input,
            doc=(
                "Temperature during the experiment (in "
                + "Kelvins) if temperature is not given in the sample logs "
                + "or needs to be overriden."
            ),
        )
        self.declareProperty("EnableDWF", True, "Enable or disable the Debye-Waller correction.")

    def validateInputs(self):
        """Validate the inputs."""
        issues = dict()
        inws = self.getProperty("VanadiumWorkspace").value
        run = inws.getRun()

        if not run.hasProperty("wavelength"):
            issues["VanadiumWorkspace"] = "Input workspace must have " + "wavelength sample log."
        else:
            try:
                float(run.getProperty("wavelength").value)
            except ValueError:
                issues["VanadiumWorkspace"] = "Invalid value for " + "wavelength sample log. " + "Wavelength must be a number."

        table = self.getProperty("EPPTable").value
        if table.rowCount() != inws.getNumberHistograms():
            issues["EPPTable"] = "Number of rows in the table must match " + "to the input workspace dimension."
        # table must have 'PeakCentre' and 'Sigma' columns
        if "PeakCentre" not in table.getColumnNames():
            issues["EPPTable"] = "EPP Table must have the PeakCentre column."
        if "Sigma" not in table.getColumnNames():
            issues["EPPTable"] = "EPP Table must have the Sigma column."

        return issues

    def get_temperature(self):
        """Return the temperature."""
        temperatureProperty = self.getProperty("Temperature")
        if not temperatureProperty.isDefault:
            return temperatureProperty.value
        temperatureLogName = "temperature"
        instrument = self.vanaws.getInstrument()
        LOG_ENTRY = "temperature_log_entry"
        if instrument.hasParameter(LOG_ENTRY, False):
            temperatureLogName = instrument.getStringParameter(LOG_ENTRY)[0]
        run = self.vanaws.getRun()
        if not run.hasProperty(temperatureLogName):
            self.log().warning(
                "No Temperature given and the 'temperature' "
                + "sample log is not present in "
                + self.vanaws.name()
                + ". T = {}K is assumed for Debye-Waller factor.".format(self.defaultT)
            )
            return self.defaultT
        try:
            temperature = run.getProperty(temperatureLogName)
            if isinstance(temperature, FloatTimeSeriesProperty):
                temperature = run.getTimeAveragedValue(temperatureLogName)
            else:
                temperature = float(temperature.value)
            return temperature
        except ValueError as err:
            self.log().warning(
                "Error of getting temperature from the "
                + "sample log "
                + err
                + ". T = {}K is assumed ".format(self.defaultT)
                + "for Debye-Waller factor."
            )
            return self.defaultT

    def PyExec(self):
        """Main execution body."""

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
        width = 3.0 * 2.0 * np.sqrt(2.0 * np.log(2.0))
        integrate.setProperty("HalfWidthInSigmas", width)
        integrate.execute()
        prog_reporter.report("Computing DWFs")
        outws = integrate.getProperty("OutputWorkspace").value
        prog_reporter.report("Applying DWFs")
        if self.getProperty("EnableDWF").value:
            # calculate array of Debye-Waller factors
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
        Calculate Debye-Waller factor according to
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
        self.log().debug("Using T = {}K for the Debye-Waller factor.".format(temperature))
        # Wavelength, Angstrom
        wlength = float(run.getLogData("wavelength").value)
        # Vanadium mass, kg
        mass_vana = 0.001 * self.Mvan / sp.constants.N_A
        temp_ratio = temperature / self.DebyeT

        if temp_ratio < 1.0e-3:
            integral = 0.5
        else:
            integral = integrate.quad(lambda x: x / np.tanh(0.5 * x / temp_ratio), 0, 1)[0]

        msd = 3.0 * sp.constants.hbar**2 / (2.0 * mass_vana * sp.constants.k * self.DebyeT) * integral * 1.0e20
        return np.exp(-msd * (4.0 * sp.constants.pi * np.sin(thetasort) / wlength) ** 2)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(ComputeCalibrationCoefVan)
