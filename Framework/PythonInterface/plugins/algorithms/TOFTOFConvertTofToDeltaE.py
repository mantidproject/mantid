from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, MatrixWorkspaceProperty,\
    PropertyMode, Progress, WorkspaceUnitValidator, InstrumentValidator, ITableWorkspaceProperty
from mantid.kernel import Direction, CompositeValidator
import mantid.simpleapi as api
import numpy as np
import scipy as sp


class TOFTOFConvertTofToDeltaE(PythonAlgorithm):
    """ Calculate energy transfer using elastic tof
    """
    logs_to_check = ['channel_width', 'EPP', 'TOF1']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)

    def category(self):
        """ Return category
        """
        return "Workflow\\MLZ\\TOFTOF;Transforms\\Units;CorrectionFunctions"

    def name(self):
        """ Return name
        """
        return "TOFTOFConvertTofToDeltaE"

    def summary(self):
        return "Converts X-axis units from TOF to energy transfer dE."

    def PyInit(self):
        """ Declare properties
        """
        validator = CompositeValidator()
        validator.add(WorkspaceUnitValidator("TOF"))
        validator.add(InstrumentValidator())

        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input, validator=validator),
                             doc="Input Sample workspace")
        # Optional but required if tof_elastic must be taken from fitted Vanadium or sample data
        self.declareProperty(ITableWorkspaceProperty("EPPTable", "", direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             "Input EPP table (optional). May be produced by FindEPP algorithm.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             "Name of the workspace that will contain the result")

        return

    def validateInputs(self):
        issues = dict()

        eppws_name = self.getPropertyValue("EPPTable")
        inws = self.getProperty("InputWorkspace").value

        # if EPP table is given, the dimensions must match
        if eppws_name:
            table = self.getProperty("EPPTable").value
            if table.rowCount() != inws.getNumberHistograms():
                issues['EPPTable'] = "Number of rows in the table must match to the input workspace dimension."
            # table must have 'PeakCentre' column
            if not 'PeakCentre' in table.getColumnNames():
                issues['EPPTable'] = "EPP Table must have the PeakCentre column."

        # input workspace must have required sample logs
        err = api.CheckForSampleLogs(inws, ",".join(self.logs_to_check))
        if err:
            issues['InputWorkspace'] = err

        # if EPP will be taken from sample logs, it must be > 0
        epp = inws.getRun().getProperty('EPP').value
        if float(epp) <= 0:
            issues['InputWorkspace'] = "EPP must be a positive number."

        return issues

    def PyExec(self):
        """ Main execution body
        """
        input_ws   = self.getProperty("InputWorkspace").value
        outws_name = self.getPropertyValue("OutputWorkspace")
        eppws_name = self.getPropertyValue("EPPTable")

        run = input_ws.getRun()
        nb_hist = input_ws.getNumberHistograms()

        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=nb_hist + 1)  # call below when summing

        # find elastic time channel and t1
        channel_width = float(run.getLogData('channel_width').value)
        tof1 = float(run.getLogData('TOF1').value)
        # t_el = epp_channel_number*channel_width + xmin
        t_el_default = float(run.getLogData('EPP').value)*channel_width + input_ws.readX(0)[0] - tof1

        if not eppws_name:
            # tof_elastic from header of raw datafile start guess on peak position
            tof_elastic = np.zeros(nb_hist)
            tof_elastic.fill(t_el_default)
            self.log().information("No EPP table is given. EPP will be taken from the sample log.")
        else:
            eppws = self.getProperty("EPPTable").value
            tof_elastic = np.array(eppws.column('PeakCentre')) - tof1


        self.log().debug("Tel = " + str(tof_elastic))

        outws = api.CloneWorkspace(input_ws,OutputWorkspace=outws_name)

        # mask detectors with EPP=0
        zeros = np.where(tof_elastic == 0)[0]
        if len(zeros) > 0:
            self.log().warning("Detectors " + str(zeros) + " have EPP=0 and will be masked.")
            api.MaskDetectors(outws, DetectorList=zeros)
            # makes sence to convert units even for masked detectors, take EPP guess for that
            for idx in zeros:
                tof_elastic[idx] = t_el_default

        instrument = outws.getInstrument()
        sample = instrument.getSample()

        factor = sp.constants.m_n*1e+15/sp.constants.eV

        # calculate new values for dataX and data Y
        for idx in range(nb_hist):
            prog_reporter.report("Setting %dth spectrum" % idx)
            det = instrument.getDetector(outws.getSpectrum(idx).getDetectorIDs()[0])
            xbins = input_ws.readX(idx) - tof1                  # take bin boundaries
            tof = xbins[:-1] + 0.5*channel_width                # take middle of each bin
            sdd = det.getDistance(sample)
            # calculate new I = t^3*I(t)/(factor*sdd^2*dt)
            dataY = input_ws.readY(idx)*tof**3/(factor*channel_width*sdd*sdd)
            dataE = input_ws.readE(idx)*tof**3/(factor*channel_width*sdd*sdd)
            outws.setY(idx, dataY)
            outws.setE(idx, dataE)
            # calculate dE = factor*0.5*sdd^2*(1/t_el^2 - 1/t^2)
            dataX = 0.5*factor*sdd*sdd*(1/tof_elastic[idx]**2 - 1/xbins**2)
            outws.setX(idx, dataX)

        outws.getAxis(0).setUnit('DeltaE')
        outws.setDistribution(True)

        self.setProperty("OutputWorkspace", outws)

# Register algorithm with Mantid.
AlgorithmFactory.subscribe(TOFTOFConvertTofToDeltaE)
