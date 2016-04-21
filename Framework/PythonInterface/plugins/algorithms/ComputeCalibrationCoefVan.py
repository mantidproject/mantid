from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty, Progress, InstrumentValidator, ITableWorkspaceProperty
from mantid.kernel import Direction
import numpy as np
from scipy import integrate
import scipy as sp


class ComputeCalibrationCoefVan(PythonAlgorithm):
    """ Calculate coefficients to normalize by Vanadium and correct Debye Waller factor
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
        return "Calculate coefficients for detector efficiency correction using the Vanadium data."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty("VanadiumWorkspace", "", direction=Direction.Input,
                                                     validator=InstrumentValidator()),
                             "Input Vanadium workspace")
        self.declareProperty(ITableWorkspaceProperty("EPPTable", "", direction=Direction.Input),
                             "Input EPP table. May be produced by FindEPP algorithm.")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             "Name the workspace that will contain the calibration coefficients")
        return

    def validateInputs(self):
        issues = dict()
        inws = self.getProperty("VanadiumWorkspace").value
        run = inws.getRun()

        if not run.hasProperty('wavelength'):
            issues['VanadiumWorkspace'] = "Input workspace must have wavelength sample log."
        else:
            try:
                float(run.getProperty('wavelength').value)
            except ValueError:
                issues['VanadiumWorkspace'] = "Invalid value for wavelength sample log. Wavelength must be a number."

        table = self.getProperty("EPPTable").value
        if table.rowCount() != inws.getNumberHistograms():
            issues['EPPTable'] = "Number of rows in the table must match to the input workspace dimension."
        # table must have 'PeakCentre' and 'Sigma' columns
        if not 'PeakCentre' in table.getColumnNames():
            issues['EPPTable'] = "EPP Table must have the PeakCentre column."
        if not 'Sigma' in table.getColumnNames():
            issues['EPPTable'] = "EPP Table must have the Sigma column."

        return issues

    def get_temperature(self):
        """
        tries to get temperature from the sample logs
        in the case of fail, default value is returned
        """
        run = self.vanaws.getRun()
        if not run.hasProperty('temperature'):
            self.log().warning("Temperature sample log is not present in " + self.vanaws.getName() +
                               " T=293K is assumed for Debye-Waller factor.")
            return self.defaultT
        try:
            temperature = float(run.getProperty('temperature').value)
        except ValueError, err:
            self.log().warning("Error of getting temperature: " + err +
                               " T=293K is assumed for Debye-Waller factor.")
            return self.defaultT

        return temperature

    def PyExec(self):
        """ Main execution body
        """

        self.vanaws = self.getProperty("VanadiumWorkspace").value       # returns workspace instance
        outws_name = self.getPropertyValue("OutputWorkspace")           # returns workspace name (string)
        eppws = self.getProperty("EPPTable").value
        nhist = self.vanaws.getNumberHistograms()
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=nhist+1)

        # calculate array of Debye-Waller factors
        dwf = self.calculate_dwf()

        # for each detector: fit gaussian to get peak_centre and fwhm
        # sum data in the range [peak_centre - 3*fwhm, peak_centre + 3*fwhm]
        dataX = self.vanaws.readX(0)
        coefY = np.zeros(nhist)
        coefE = np.zeros(nhist)
        instrument = self.vanaws.getInstrument()
        detID_offset = self.get_detID_offset()
        peak_centre = eppws.column('PeakCentre')
        sigma = eppws.column('Sigma')

        for idx in range(nhist):
            prog_reporter.report("Setting %dth spectrum" % idx)
            dataY = self.vanaws.readY(idx)
            det = instrument.getDetector(idx + detID_offset)
            if np.max(dataY) == 0 or det.isMasked():
                coefY[idx] = 0.
                coefE[idx] = 0.
            else:
                dataE = self.vanaws.readE(idx)
                fwhm = sigma[idx]*2.*np.sqrt(2.*np.log(2.))
                idxmin = (np.fabs(dataX-peak_centre[idx]+3.*fwhm)).argmin()
                idxmax = (np.fabs(dataX-peak_centre[idx]-3.*fwhm)).argmin()
                coefY[idx] = dwf[idx]*sum(dataY[idxmin:idxmax+1])
                coefE[idx] = dwf[idx]*sum(dataE[idxmin:idxmax+1])

        # create X array, X data are the same for all detectors, so
        coefX = np.zeros(nhist)
        coefX.fill(dataX[0])

        create = self.createChildAlgorithm("CreateWorkspace")
        create.setPropertyValue('OutputWorkspace', outws_name)
        create.setProperty('ParentWorkspace', self.vanaws)
        create.setProperty('DataX', coefX)
        create.setProperty('DataY', coefY)
        create.setProperty('DataE', coefE)
        create.setProperty('NSpec', nhist)
        create.setProperty('UnitX', 'TOF')
        create.execute()
        outws = create.getProperty('OutputWorkspace').value

        self.setProperty("OutputWorkspace", outws)

    def get_detID_offset(self):
        """
        returns ID of the first detector
        """
        return self.vanaws.getSpectrum(0).getDetectorIDs()[0]

    def calculate_dwf(self):
        """
        Calculates Debye-Waller factor according to
        Sears and Shelley Acta Cryst. A 47, 441 (1991)
        """
        run = self.vanaws.getRun()
        nhist = self.vanaws.getNumberHistograms()
        thetasort = np.zeros(nhist)  # theta in radians, not 2Theta

        instrument = self.vanaws.getInstrument()
        detID_offset = self.get_detID_offset()

        for i in range(nhist):
            det = instrument.getDetector(i + detID_offset)
            thetasort[i] = 0.5*np.sign(np.cos(det.getPhi()))*self.vanaws.detectorTwoTheta(det)
            # thetasort[i] = 0.5*self.vanaws.detectorSignedTwoTheta(det) # gives opposite sign for detectors 0-24

        temperature = self.get_temperature()                    # T in K
        wlength = float(run.getLogData('wavelength').value)     # Wavelength, Angstrom
        mass_vana = 0.001*self.Mvan/sp.constants.N_A            # Vanadium mass, kg
        temp_ratio = temperature/self.DebyeT

        if temp_ratio < 1.e-3:
            integral = 0.5
        else:
            integral = integrate.quad(lambda x: x/sp.tanh(0.5*x/temp_ratio), 0, 1)[0]

        msd = 3.*sp.constants.hbar**2/(2.*mass_vana*sp.constants.k * self.DebyeT)*integral*1.e20
        return np.exp(-msd*(4.*sp.pi*sp.sin(thetasort)/wlength)**2)


# Register algorithm with Mantid.
AlgorithmFactory.subscribe(ComputeCalibrationCoefVan)
