#pylint: disable=no-init,invalid-name
from mantid.simpleapi import *
from mantid.api import *
from mantid.kernel import *
import math
import numpy

class USANSSimulation(PythonAlgorithm):

    def category(self):
        return "SANS"

    def name(self):
        return "USANSSimulation"

    def summary(self):
        return "Simulate a USANS workspace"

    def PyInit(self):
        self.declareProperty("TwoTheta", 0.01, "Scattering angle in degrees")
        self.declareProperty(FloatArrayProperty("WavelengthPeaks", values=[0.72, 0.9, 1.2, 1.8, 3.6],\
                             direction=Direction.Input), "Wavelength peaks out of the monochromator")
        self.declareProperty("CountTime", 1000.0, "Fake count time")

        # Model parameters
        self.declareProperty("EmptyRun", False, "If True, the run is considered an empty run")
        self.declareProperty("SphereRadius", 60.0, "Radius for the sphere model (Angstrom)")
        self.declareProperty("Background", 0.0, "Background")
        self.declareProperty("SigmaPeak", 0.01, "Width of the wavelength peaks")

        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")
        self.declareProperty(MatrixWorkspaceProperty("MonitorWorkspace", "", Direction.Output), "Output monitor workspace")

    def PyExec(self):
        workspace = self.getPropertyValue("OutputWorkspace")
        out_ws = CreateSimulationWorkspace(Instrument="USANS",
                                           BinParams="0,50,32000",
                                           UnitX="TOF",
                                           OutputWorkspace=workspace)
        out_ws.setYUnitLabel("1/cm")

        data_x = out_ws.dataX(0)
        mon_ws_name = self.getPropertyValue("MonitorWorkspace")
        mon_ws = CreateWorkspace(dataX=data_x, dataY=numpy.zeros(len(data_x)-1),
                                 UnitX="TOF", OutputWorkspace=mon_ws_name)
        mon_y = mon_ws.dataY(0)
        mon_e = mon_ws.dataE(0)

        # Number of pixels for the main detector
        n_pixels = out_ws.getNumberHistograms()/2
        # Clean up the workspace
        for j in range(n_pixels):
            data_y = out_ws.dataY(j)
            for i in range(len(data_y)):
                data_y[i] = 0.0

        # Fill monitor workspace with fake beam profile
        count_time = self.getProperty("CountTime").value
        for i in range(len(data_x)-1):
            wl_i = 0.0039560/30.0*(data_x[i]+data_x[i+1])/2.0
            mon_y[i] = count_time*math.exp(-wl_i)
            mon_e[i] = math.sqrt(mon_y[i])

        # Add analyzer theta value and monochromator angle theta_b in logs
        two_theta = self.getProperty("TwoTheta").value
        is_empty_run = self.getProperty("EmptyRun").value
        if is_empty_run:
            two_theta = 0.0

        theta_b = 70.0
        theta = theta_b + two_theta
        out_ws.getRun().addProperty("AnalyzerTheta", theta, 'degree', True)
        out_ws.getRun().addProperty("two_theta", two_theta, 'degree', True)
        out_ws.getRun().addProperty("MonochromatorTheta", theta_b, 'degree', True)
        out_ws.getRun().addProperty("run_title", "Simulated USANS", True)
        out_ws.getRun().addProperty("run_number", "1234", True)

        # List of wavelength peaks, and width of the peaks
        wl_peaks = self.getProperty("WavelengthPeaks").value
        sigma = self.getProperty("SigmaPeak").value

        for wl in wl_peaks:
            q = 6.28*math.sin(two_theta)/wl
            Logger("USANS").notice( "wl = %g; Q = %g" % (wl, q))

            for i in range(len(data_x)-1):
                wl_i = 0.0039560/30.0*(data_x[i]+data_x[i+1])/2.0

                # Scale the I(q) by a Gaussian to simulate the wavelength peaks selected by the monochromator
                flux = 1.0e6/(sigma*math.sqrt(2.0*math.pi))*math.exp(-(wl_i-wl)*(wl_i-wl)/(2.0*sigma*sigma))

                # Multiply by beam profile
                flux *= mon_y[i]

                # Account for transmission
                if not is_empty_run:
                    flux *= math.exp(-wl_i/2.0)

                # Transmission detector
                for j in range(n_pixels, 2*n_pixels):
                    det_pos = out_ws.getInstrument().getDetector(j).getPos()
                    r = math.sqrt(det_pos.Y()*det_pos.Y()+det_pos.X()*det_pos.X())
                    sigma = 0.01
                    scale = math.exp(-r*r/(2.0*sigma*sigma))
                    data_y = out_ws.dataY(j)
                    data_y[i] += int(scale*flux)
                    data_e = out_ws.dataE(j)
                    data_e[i] = math.sqrt(data_e[i]*data_e[i]+scale*scale*flux*flux)

                # If we have an empty run, there's no need to fill the main detector
                if is_empty_run:
                    continue

                # Compute I(q) and store the results
                q_i = q*wl/wl_i
                i_q = self._sphere_model(q_i, scale=flux)

                for j in range(n_pixels):
                    det_pos = out_ws.getInstrument().getDetector(j).getPos()
                    r = math.sqrt(det_pos.Y()*det_pos.Y()+det_pos.X()*det_pos.X())
                    sigma = 0.01
                    scale = math.exp(-r*r/(2.0*sigma*sigma))

                    data_y = out_ws.dataY(j)
                    data_y[i] += int(i_q*scale)
                    data_e = out_ws.dataE(j)
                    data_e[i] = math.sqrt(data_e[i]*data_e[i]+i_q*i_q*scale*scale)

        self.setProperty("OutputWorkspace", out_ws)
        self.setProperty("MonitorWorkspace", mon_ws)

    def _sphere_model(self, q, scale):
        """
            Return I(q) for a sphere model
            @param q: q-value
            @param scale: normalization factor to give I(q)
        """
        radius = self.getProperty("SphereRadius").value
        bck = self.getProperty("Background").value
        qr = q*radius
        bes = 3.0*(math.sin(qr)-qr*math.cos(qr))/(qr*qr*qr) if not qr == 0.0 else 1.0
        vol = 4.0*math.pi/3.0*radius*radius*radius
        f2 = vol*bes*bes*1.0e-6
        return(scale*f2+bck)

#############################################################################################
AlgorithmFactory.subscribe(USANSSimulation())
