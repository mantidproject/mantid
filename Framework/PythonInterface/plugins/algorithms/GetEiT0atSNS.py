#pylint: disable=no-init,invalid-name
import mantid
import numpy


class GetEiT0atSNS(mantid.api.PythonAlgorithm):
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Inelastic"

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
                v=437.4*numpy.sqrt(wm.getRun()['EnergyRequest'].getStatistics().mean)
                t1=m1.distance(so)*1e6/v
                t2=m2.distance(so)*1e6/v
                t1f=int(t1*60e-6) #frame number for monitor 1
                t2f=int(t2*60e-6) #frame number for monitor 2
                wtemp=mantid.simpleapi.ChangeBinOffset(wm,t1f*16667,sp1,sp1)
                wtemp=mantid.simpleapi.ChangeBinOffset(wtemp,t2f*16667,sp2,sp2)
                wtemp=mantid.simpleapi.Rebin(InputWorkspace=wtemp,Params="1",PreserveEvents=True)
                #Run GetEi algorithm
                alg=mantid.simpleapi.GetEi(InputWorkspace=wtemp,Monitor1Spec=sp1+1,Monitor2Spec=sp2+1,EnergyEstimate=EGuess)
                Ei=alg[0]
                Tzero=alg[3]                                        #Extract incident energy and T0
                mantid.simpleapi.DeleteWorkspace(wtemp)
            except Exception as e:
                raise RuntimeError("Could not get Ei, and this is not a white beam run\n"+e.message)
        self.setProperty("Ei",Ei)
        self.setProperty("T0",Tzero)

mantid.api.AlgorithmFactory.subscribe(GetEiT0atSNS)
