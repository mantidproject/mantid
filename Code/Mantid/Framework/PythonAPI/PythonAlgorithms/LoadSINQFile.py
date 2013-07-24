#--------------------------------------------------------------
# Algorithm which loads a SINQ file. It matches the instrument 
# and the right dictionary file and then goes away and calls 
# LoadFlexiNexus and others to load the file.
#
# Mark Koennecke, November 2012
#--------------------------------------------------------------
from mantid.api import PythonAlgorithm, registerAlgorithm, WorkspaceFactory, FileProperty, FileAction, WorkspaceProperty, FrameworkManager
from mantid.kernel import Direction, StringListValidator, ConfigServiceImpl
import mantid.simpleapi
import MantidFramework 

#--------- place to look for dictionary files
dictsearch='/home/christophe/poldi/dev/mantid-2.3.2-Source/PSIScripts'

class LoadSINQFile(PythonAlgorithm):
    def category(self):
        return "DataHandling;PythonAlgorithms"

    def PyInit(self):
        global dictsearch
        instruments=["AMOR","BOA","DMC","FOCUS","HRPT","MARSI","MARSE","POLDI",
                     "RITA-2","SANS","SANS2","TRICS"]
        self.declareProperty("Instrument","AMOR",
                             StringListValidator(instruments),
                             "Choose Instrument",direction=Direction.Input)
        self.declareProperty(FileProperty(name="Filename",defaultValue="",
                                          action=FileAction.Load, extensions=["h5","hdf"]))
        self.declareProperty("OutputWorkspace","nexus")
        dictsearch = ConfigServiceImpl.Instance().getInstrumentDirectory()

    def PyExec(self):
        inst=self.getProperty('Instrument').value
        fname =self.getProperty('Filename').value
        self.log().error('Running LoadSINQFile for ' + inst)


        if inst =="AMOR":
            self.doAmor()
        elif inst == "BOA":
            self.doBoa()
        elif inst == "DMC":
            self.doDMC()
        elif inst == "FOCUS":
            self.doFocus()
        elif inst == "HRPT":
            self.doHRPT()
        elif inst == "MARSI":
            self.doMarsInelastic()
        elif inst == "MARSE":
            self.doMarsElastic()
        elif inst == "POLDI":
            self.doPoldi()
        elif inst == "RITA-2":
            self.doRITA()
        elif inst == "SANS":
            self.doSANS()
        elif inst == "SANS2":
            self.doSANS()
        elif inst == "TRICS":
            self.doTRICS()
        else: 
            pass
        
    def doAmor(self):
        dicname = dictsearch +"/mantidamor.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doBoa(self):
        dicname = dictsearch +"/mantidboa.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doDMC(self):
        self.log().debug('dictsearch in doDMC: ' + dictsearch)
        dicname = dictsearch +"/mantiddmc.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doHRPT(self):
        dicname = dictsearch +"/mantidhrpt.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doFocus(self):
        dicname = dictsearch +"/mantidfocus.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doMarsInelastic(self):
        dicname = dictsearch +"/mantidmarsin.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doMarsElastic(self):
        dicname = dictsearch +"/mantidmarse.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doPoldi(self):
        dicname = dictsearch +"/mantidpoldi.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
#        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,'tmp')
        mapname= dictsearch +'/poldigroup.xml'
        exec(wname + '= mantid.simpleapi.GroupDetectors(\'tmp\',mapname,[],[],[],False,\'Sum\',True)')
        mantid.simpleapi.DeleteWorkspace('tmp')
        
    def doSANS(self):
        dicname = dictsearch +"/mantidsans.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
    def doTRICS(self):
        dicname = dictsearch +"/mantidtrics.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,'tmp')
        exec(wname + '= mantid.simpleapi.Transpose3D(\'tmp\',\'TRICS\')')
        mantid.simpleapi.DeleteWorkspace('tmp')
    def doRITA(self):
        dicname = dictsearch +"/mantidrita.dic"
        fname =self.getProperty('Filename').value 
        wname =self.getProperty('OutputWorkspace').value
        ws = mantid.simpleapi.LoadFlexiNexus(fname,dicname,wname)
         

#---------- register with Mantid
registerAlgorithm(LoadSINQFile())
