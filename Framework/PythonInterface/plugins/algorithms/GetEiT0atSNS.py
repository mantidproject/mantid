# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=no-init,invalid-name
from __future__ import (absolute_import, division, print_function)
import mantid
import numpy


class GetEiT0atSNS(mantid.api.PythonAlgorithm):
    def category(self):
        """ Return category
        """
        return "Inelastic\\Ei"

    def seeAlso(self):
        return [ "GetEi" ]

    def name(self):
        """ Return name
        """
        return "GetEiT0atSNS"

    def summary(self):
        """ Return summary
        """
        return "Get Ei and T0 on ARCS and SEQUOIA instruments."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(mantid.api.WorkspaceProperty("MonitorWorkspace", "",direction=mantid.kernel.Direction.InOut),
                             "Monitor workspace")
        self.declareProperty("IncidentEnergyGuess",-1.,doc="Incident energy guess")
        self.declareProperty("Ei",0.0,mantid.kernel.Direction.Output)
        self.declareProperty("T0",0.0,mantid.kernel.Direction.Output)
        return

    def PyExec(self):
        """ Main execution body
        """
        wm=self.getProperty("MonitorWorkspace").value
        i=wm.getInstrument()

        if numpy.mean(wm.getRun()['vChTrans'].value) == 2:
            Ei=numpy.nan
            Tzero=numpy.nan
        else:
            EGuess=self.getProperty("IncidentEnergyGuess").value
            if EGuess<0:
                try:
                    EGuess=wm.getRun()['EnergyRequest'].getStatistics().mean
                except:
                    raise RuntimeError("No energy guess was given or could be found in sample logs")
            try:
            #fix more than 2 monitors
                sp1=-1
                sp2=-1
                nsp=wm.getNumberHistograms()
                if nsp < 2:
                    raise ValueError("There are less than 2 monitors")
                for sp in range(nsp):
                    if wm.getSpectrum(sp).getDetectorIDs()[0]==-int(i.getNumberParameter('ei-mon1-spec')[0]):
                        sp1=sp
                    if wm.getSpectrum(sp).getDetectorIDs()[0]==-int(i.getNumberParameter('ei-mon2-spec')[0]):
                        sp2=sp
                if sp1==-1:
                    raise RuntimeError("Could not find spectrum for the first monitor")
                if sp2==-1:
                    raise RuntimeError("Could not find spectrum for the second monitor")
                #change frame for monitors. ARCS monitors would be in the first frame for Ei>10meV
                so=i.getSource().getPos()
                m1=wm.getDetector(sp1).getPos()
                m2=wm.getDetector(sp2).getPos()
                run_starttime = wm.getRun().startTime()
                from mantid.kernel import DateAndTime
                SNS_DAS_changed_time_wrapping = DateAndTime("2019-06-15T00:00:00")
                if run_starttime < SNS_DAS_changed_time_wrapping:
                    v=437.4*numpy.sqrt(wm.getRun()['EnergyRequest'].getStatistics().mean)
                    t1=m1.distance(so)*1e6/v
                    t2=m2.distance(so)*1e6/v
                    t1f=int(t1*60e-6) #frame number for monitor 1
                    t2f=int(t2*60e-6) #frame number for monitor 2
                    wtemp=mantid.simpleapi.ChangeBinOffset(wm,t1f*16667,sp1,sp1)
                    wtemp=mantid.simpleapi.ChangeBinOffset(wtemp,t2f*16667,sp2,sp2)
                else:
                    wtemp = wm
                maxtof = wtemp.readX(0)[-1]
                period = 1.e6/60
                Nmax = int(maxtof/period) + 1
                for i in range(1,Nmax+1):
                    tmin = min(i*period-30., maxtof)
                    tmax = min(i*period+30., maxtof)
                    if tmin<tmax:
                        mantid.simpleapi.MaskBins(InputWorkspace=wtemp, OutputWorkspace=wtemp, XMin=tmin, XMax=tmax)
                wtemp=mantid.simpleapi.Rebin(InputWorkspace=wtemp,Params="1",PreserveEvents=True)
                #Run GetEi algorithm
                alg=mantid.simpleapi.GetEi(InputWorkspace=wtemp,Monitor1Spec=sp1+1,Monitor2Spec=sp2+1,EnergyEstimate=EGuess)
                Ei=alg[0]
                Tzero=alg[3]                                        #Extract incident energy and T0
                mantid.simpleapi.DeleteWorkspace(wtemp)
            except Exception as e:
                raise RuntimeError("Could not get Ei, and this is not a white beam run\n"+str(e))
        self.setProperty("Ei",Ei)
        self.setProperty("T0",Tzero)


mantid.api.AlgorithmFactory.subscribe(GetEiT0atSNS)
