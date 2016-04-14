from mantid.api import PythonAlgorithm, AlgorithmFactory, WorkspaceProperty, MatrixWorkspaceProperty,\
    PropertyMode, Progress
from mantid.kernel import Direction, StringListValidator
import mantid.simpleapi as api
import numpy as np
import scipy as sp


class TOFTOFConvertTofToDeltaE(PythonAlgorithm):
    """ Calculate energy transfer using elastic tof
    """
    logs_to_check = ['channel_width', 'chopper_ratio', 'chopper_speed', 'Ei', 'wavelength', 'EPP']

    def __init__(self):
        """
        Init
        """
        PythonAlgorithm.__init__(self)

    def category(self):
        """ Return category
        """
        return "PythonAlgorithms\\MLZ\\TOFTOF;Transforms\\Units;CorrectionFunctions"

    def name(self):
        """ Return name
        """
        return "TOFTOFConvertTofToDeltaE"

    def summary(self):
        return "Converts X-axis units from TOF to energy transfer dE."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", direction=Direction.Input),
                             "Input Sample workspace")
        # Optional but required if choice of tof_elastic from Vanadium ws or file
        self.declareProperty(MatrixWorkspaceProperty("VanadiumWorkspace", "", direction=Direction.Input,
                                                     optional=PropertyMode.Optional),
                             "Input Vanadium workspace (optional)")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output),
                             "Name the workspace that will contain the result")
        choice_tof = ["Geometry", "FitVanadium", "FitSample"]
        self.declareProperty("ChoiceElasticTof", "Geometry", StringListValidator(choice_tof))
        return

    def validateInputs(self):
        issues = dict()

        choice_tof = self.getProperty("ChoiceElasticTof").value
        vanaws_name = self.getPropertyValue("VanadiumWorkspace")
        inws = self.getProperty("InputWorkspace").value

        # instrument must be set, this can be moved to validator
        # after validators will work for WorkspaceGroups
        if not inws.getInstrument().getName():
            issues['InputWorkspace'] = "Instrument must be set."

        # X units must be TOF
        xunit = inws.getAxis(0).getUnit().unitID()
        if xunit != 'TOF':
            issues['InputWorkspace'] = "X axis units must be TOF. "

        # input workspace must have required sample logs
        err = api.CheckForSampleLogs(inws, ",".join(self.logs_to_check))
        if err:
            issues['InputWorkspace'] = err

        # if Vanadium will be used to find EPP, workspace must exist and have required sample logs
        if choice_tof == 'FitVanadium':
            if not api.AnalysisDataService.doesExist(vanaws_name):
                issues['VanadiumWorkspace'] = "Valid Vanadium Workspace must be given for choosen ElasticTof option."

            else:
                vanaws = api.AnalysisDataService.retrieve(vanaws_name)
                xunit = vanaws.getAxis(0).getUnit().unitID()
                if xunit != 'TOF':
                    issues['VanadiumWorkspace'] = "X axis units must be TOF. "
                err = api.CheckForSampleLogs(vanaws, ",".join(self.logs_to_check))
                if err:
                    issues['VanadiumWorkspace'] = err
                # required sample logs for sample and vanadium must be identical
                else:
                    result = api.CompareSampleLogs([inws.getName(), vanaws_name], self.logs_to_check, 0.01)
                    if len(result) > 0:
                        issues['VanadiumWorkspace'] = "Sample logs " + result + " must match for Vanadium and sample workspaces."
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
        choice_tof = self.getProperty("ChoiceElasticTof").value

        run = input_ws.getRun()
        nb_hist = input_ws.getNumberHistograms()

        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=nb_hist + 1)  # extra call below when summing

        # find elastic time channel
        tof_elastic = np.zeros(nb_hist)
        channel_width = float(run.getLogData('channel_width').value)
        # t_el = epp_channel_number*channel_width + xmin
        t_el_default = float(run.getLogData('EPP').value)*channel_width + input_ws.readX(0)[0]

        if choice_tof == 'Geometry':
            # tof_elastic from header of raw datafile start guess on peak position
            tof_elastic.fill(t_el_default)

        if choice_tof == 'FitSample':
            prog_reporter.report("Fit function")
            for idx in range(nb_hist):
                tof_elastic[idx] = api.FitGaussian(input_ws, idx)[0]

        if choice_tof == 'FitVanadium':
            vanaws = self.getProperty("VanadiumWorkspace").value
            prog_reporter.report("Fit function")
            for idx in range(nb_hist):
                tof_elastic[idx] = api.FitGaussian(vanaws, idx)[0]

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
        length1 = instrument.getSource().getPos().distance(sample.getPos())
        self.log().debug("Sample-source distance: " + str(length1))
        factor = sp.constants.m_n*1e+15/sp.constants.eV

        # calculate new values for dataX and data Y
        for idx in range(nb_hist):
            prog_reporter.report("Setting %dth spectrum" % idx)
            det = instrument.getDetector(outws.getSpectrum(idx).getDetectorIDs()[0])
            xbins = input_ws.readX(idx)                   # take bin boundaries
            tof = xbins[:-1] + 0.5*channel_width          # take middle of each bin
            sdd = det.getDistance(sample) + length1       # det.getDistance(source) is different for different detectors
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
