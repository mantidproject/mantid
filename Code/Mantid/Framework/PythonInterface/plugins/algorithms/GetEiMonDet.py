#pylint: disable=no-init,invalid-name
from mantid.api import PythonAlgorithm, AlgorithmFactory,WorkspaceProperty
from mantid.kernel import Direction,IntBoundedValidator,FloatBoundedValidator
import mantid.simpleapi
import mantid
from numpy import array,where

class GetEiMonDet(PythonAlgorithm):
    """ Get incident energy from a monitor and some detectors
    """
    def category(self):
        """ Return category
        """
        return "PythonAlgorithms;Inelastic"

    def name(self):
        """ Return name
        """
        return "GetEiMonDet"

    def summary(self):
        """ Return summary
        """
        return "Get incident energy from one monitor and some detectors."

    def PyInit(self):
        """ Declare properties
        """
        self.declareProperty(WorkspaceProperty("DetectorWorkspace","",Direction.Input),"Workspace containing data from detectors")
        self.declareProperty(WorkspaceProperty("MonitorWorkspace","", Direction.Input),"Workspace containing data from monitor(s)")
        self.declareProperty("EnergyGuess",-1.,FloatBoundedValidator(lower=0.),"Incident energy guess")
        self.declareProperty("MonitorSpectrumNumber",-1,IntBoundedValidator(lower=1),"Spectrum number of the monitor")
        self.declareProperty("MaximumDistanceFraction",1.02,FloatBoundedValidator(lower=1.0001),"Maximum distance for detectors to be considered, as fraction of minimum distance (default = 1.02)")
        self.declareProperty("IncidentEnergy",0.0,Direction.Output)
        self.declareProperty("FirstMonitorPeak",0.0,Direction.Output)
        self.declareProperty("FirstMonitorIndex",0,Direction.Output)
        self.declareProperty("Tzero",0.0,Direction.Output)
        return

    def PyExec(self):
        """ Main execution body
        """
        __w=self.getProperty("DetectorWorkspace").value
        __w_mon=self.getProperty("MonitorWorkspace").value
        mon1spec=self.getProperty("MonitorSpectrumNumber").value
        Eguess=self.getProperty("EnergyGuess").value

        #find the minimum distance, then select only detector that are at d<dmin*MaximumDistanceFraction
        sample=__w.getInstrument().getSample()
        dist=[]
        for i in range(__w.getNumberHistograms()):
            det=__w.getDetector(i)
            dist.append(det.getDistance(sample))
        dist=array(dist)
        dfrac=float(self.getProperty("MaximumDistanceFraction").value)
        inds=where(dist<dist.min()*dfrac)
        dist1=dist[inds]
        #group detectors, rebin, and append spectra to monitors
        gd=mantid.api.AlgorithmManager.createUnmanaged('GroupDetectors')
        gd.setChild(True)
        gd.initialize()
        gd.setAlwaysStoreInADS(True)
        gd.setLogging(False)
        gd.setProperty("InputWorkspace",__w.getName())
        gd.setProperty("OutputWorkspace",'__sum')
        gd.setProperty("WorkspaceIndexList",inds[0])
        gd.setProperty("PreserveEvents",'1')
        gd.execute()

        #mantid.simpleapi.GroupDetectors(InputWorkspace=__w.getName(),OutputWorkspace='__sum',WorkspaceIndexList=inds[0],PreserveEvents='1')
        rw=mantid.api.AlgorithmManager.createUnmanaged("RebinToWorkspace")
        rw.setChild(True)
        rw.initialize()
        rw.setAlwaysStoreInADS(True)
        rw.setLogging(False)
        rw.setProperty("WorkspaceToRebin",'__sum')
        rw.setProperty("WorkspaceToMatch",__w_mon.getName())
        rw.setProperty("OutputWorkspace",'__sum')
        rw.setProperty("PreserveEvents",'0')
        rw.execute()


        #mantid.simpleapi.RebinToWorkspace(WorkspaceToRebin='__sum',WorkspaceToMatch=__w_mon.getName(),OutputWorkspace='__sum',PreserveEvents='0')
        ap=mantid.api.AlgorithmManager.createUnmanaged("AppendSpectra")
        ap.setChild(True)
        ap.setLogging(False)
        ap.initialize()
        ap.setAlwaysStoreInADS(True)
        ap.setProperty("InputWorkspace1",__w_mon.getName())
        ap.setProperty("InputWorkspace2",'__sum')
        ap.setProperty("OutputWorkspace",'__app')
        ap.execute()

        #mantid.simpleapi.AppendSpectra(InputWorkspace1=__w_mon.getName(),InputWorkspace2='__sum',OutputWorkspace='__app')


        #move detector along the beam, in the positive Z direction
        __app=mantid.mtd['__app']
        __sum=mantid.mtd['__sum']
        mon2spec=__app.getNumberHistograms()-1
        s=__app.getSpectrum(mon2spec)
        dID=s.getDetectorIDs()[0]
        s.setDetectorID(dID)
        detDist=__sum.getDetector(0).getDistance(__sum.getInstrument().getSample())

        mo=mantid.api.AlgorithmManager.createUnmanaged("MoveInstrumentComponent")
        mo.setChild(True)
        mo.setLogging(False)
        mo.initialize()
        mo.setAlwaysStoreInADS(True)
        mo.setProperty("Workspace",'__app')
        mo.setProperty("DetectorID",dID)
        mo.setProperty("Z",detDist)
        mo.setProperty("RelativePosition",'0')
        mo.execute()
        #mantid.simpleapi.MoveInstrumentComponent(Workspace='__app',DetectorID=dID,Z=detDist,RelativePosition='0')

        #fix spectrum numbers
        for i in range(__w_mon.getNumberHistograms()):
            __app.getSpectrum(i).setSpectrumNo(__w_mon.getSpectrum(i).getSpectrumNo())
        __app.getSpectrum(mon2spec).setSpectrumNo(0)

        #get Ei
        results=mantid.simpleapi.GetEi(InputWorkspace='__app',Monitor1Spec=mon1spec,Monitor2Spec=0,EnergyEstimate=Eguess)

        #cleanup
        mantid.mtd.remove('__sum')
        mantid.mtd.remove('__app')

        #return the results from GetEi
        self.setProperty("IncidentEnergy",results[0])
        self.setProperty("FirstMonitorPeak",results[1])
        self.setProperty("FirstMonitorIndex",results[2])
        self.setProperty("Tzero",results[3])
        return


AlgorithmFactory.subscribe(GetEiMonDet)
