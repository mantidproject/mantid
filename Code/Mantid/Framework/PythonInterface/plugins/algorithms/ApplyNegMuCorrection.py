from mantid.api import *  # PythonAlgorithm, registerAlgorithm, WorkspaceProperty

class ApplyNegMuCorrection(PythonAlgorithm):

    #Combining work spaces and normalising the correction.
    def combine(self,dd,runno,A2000,B2000,A3000,B3000,C,C1,spec):
        if spec<10:
            s='0'+str(spec)
        else:
            s=str(spec)
        print dd+'ral0'+runno+'.rooth30'+s+'.dat'
        #loading data
        try:
            ws3000=Load(Filename=dd+r'\ral0'+runno+'.rooth30'+s+'.dat', OutputWorkspace='ws3000')
            print 'hello'
            ws2000=Load(Filename=dd+r'\ral0'+runno+'.rooth20'+s+'.dat', OutputWorkspace='ws2000')
        except RuntimeError:
            print runno+' '+s+"not found"

        #Correcting for Gain and offset of the detectors
        ws2000_corr=CreateWorkspace(A2000*ws2000.readX(0)[:]+B2000,ws2000.readY(0)[:])
        ws3000_corr=CreateWorkspace(A3000*ws3000.readX(0)[:]+B3000,ws3000.readY(0)[:])

        #Summing total counts for normalisation
        ws2000_total=0
        ws3000_total=0
        for x in range (0,8000):
            try:
                ws2000_total=ws2000_corr.readY(0)[x]+ws2000_total
                ws3000_total=ws3000_corr.readY(0)[x]+ws3000_total
            except:
                continue

        print ws2000_total
        print ws3000_total
        #normalising
        ws2000_corr=ws2000_corr/ws2000_total
        ws3000_corr=ws3000_corr/ws3000_total

        #rebinning to add detectors together
        bin=[100,ws2000.readX(0)[2]-ws2000.readX(0)[1],8000]

        ws2000_corr_rebin=Rebin(ws2000_corr,bin)
        ws3000_corr_rebin=Rebin(ws3000_corr,bin)

        ws_ral=Plus(ws2000_corr_rebin,ws3000_corr_rebin)

        suf='_'+str(spec)+'_'+runno

        RenameWorkspaces(ws_ral,Suffix=suf)
        RenameWorkspaces(ws2000_corr,Suffix=suf)
        RenameWorkspaces(ws3000_corr,Suffix=suf)

        DeleteWorkspace(ws2000)
        DeleteWorkspace(ws3000)
        DeleteWorkspace(ws2000_corr_rebin)
        DeleteWorkspace(ws3000_corr_rebin)

        return

    def PyInit(self):
        self.declareProperty(name="Data Directory",defaultValue=r'M:\Data\Negative Muons\forMantid',doc="Data directory")
        self.declareProperty(name="First Run Number",defaultValue=1718,doc="First Run Number")
        self.declareProperty(name="Last Run Number",defaultValue=1719,doc="Last Run Number")
        self.declareProperty(name="Gain RIKEN High E",defaultValue=1.077,doc="Gain RIKEN High E")
        self.declareProperty(name="Offset RIKEN High E",defaultValue=-1,doc="OffSet RIKEN High E")
        self.declareProperty(name="Gain ISIS High E",defaultValue=1.278,doc="Gain ISIS High E")
        self.declareProperty(name="Offset ISIS High E",defaultValue=-12,doc="OffSet ISIS High E")
        self.declareProperty(name="Gain ISIS Low E",defaultValue=1.2,doc="Gain ISIS Low E")
        self.declareProperty(name="Offset ISIS Low E",defaultValue=0.0,doc="OffSet ISIS Low E")

    def category(self):
        return "CorrectionFunctions;Muon"

    def PyExec(self):

        spec=1
        dd = self.getProperty("Data Directory").value
        first = self.getProperty("First Run Number").value
        last = self.getProperty("Last Run Number").value+1
        GRHE=self.getProperty("Gain RIKEN High E").value
        ORHE=self.getProperty("Offset RIKEN High E").value
        GIHE=self.getProperty("Gain ISIS High E").value
        OIHE=self.getProperty("Offset ISIS High E").value
        GILE=self.getProperty("Gain ISIS Low E").value
        OILE=self.getProperty("Offset ISIS Low E").value

        for x in range(first,last):
           for spec in range(0,3):
               runno=str(x)
               self.combine(dd,runno,GRHE,ORHE,GIHE,OIHE,GILE,OILE,spec)
               self.combine(dd,runno,GRHE,ORHE,GIHE,OIHE,GILE,OILE,10)



AlgorithmFactory.subscribe(ApplyNegMuCorrection)
