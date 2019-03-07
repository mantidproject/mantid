"""*WIKI* 

This algorithm allows a single spectrum to be extracted from a range of workspaces and placed into a single workspace for comparison and plotting.  The LabelUsing property allows you to control what the end labels applied to each spectra will be.  The default is to use the source workspace name, but you can specify the name of a log value to use as the label, e.g. Temp_Sample.  the LabelValue property allows control of how a single value is extracted from time series logs.
Modified from standard ConjoinSpectra to make the Y axis numeric and so allow Colour Fill and Contour plots of the resulting Workspace2D. (JSL)

*WIKI*"""


from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *
import os

class ConjoinSpectraNumAx(PythonAlgorithm):
    """
    Conjoins spectra from several workspaces into a single workspaceExists
    
    Spectra to be conjoined must be equally binned in order for ConjoinSpectra to work. If necessary use RebinToWorkspace first.         
    """
   
    def category(self):
        return "Transforms\\Merging;PythonAlgorithms"

    def name(self):
        return "ConjoinSpectraNumAx"

    def PyInit(self):
        self.declareProperty("InputWorkspaces","", validator=StringMandatoryValidator(), doc="Comma seperated list of workspaces to use, group workspaces will automatically include all members.")
        self.declareProperty(WorkspaceProperty("OutputWorkspace", "", direction=Direction.Output), doc="Name the workspace that will contain the result")
        self.declareProperty("WorkspaceIndex", 0, doc="The workspace index of the spectra in each workspace to extract. Default: 0")
        self.declareProperty("LabelUsing", "", doc="The name of a log value used to label the resulting spectra. Default: The source workspace name")
        labelValueOptions =  ["Mean","Median","Maximum","Minimum","First Value","Last Value"]
        self.declareProperty("LabelValue", "Mean", validator=StringListValidator(labelValueOptions), doc="How to derive the value from a time series property")
      
      
    def PyExec(self):
        # get parameter values
        wsOutput = self.getPropertyValue("OutputWorkspace")
        wsIndex = int(self.getPropertyValue("WorkspaceIndex"))
        wsString = self.getPropertyValue("InputWorkspaces").strip()
        labelUsing = self.getPropertyValue("LabelUsing").strip()
        labelValue = self.getPropertyValue("LabelValue")

        #internal values
        wsTemp = "__ConjoinSpectra_temp"
        loopIndex=0
        
        #get the wokspace list
        wsNames = []
        for wsName in wsString.split(","):
        #check the ws is in mantid
            ws = mtd[wsName.strip()]
            #if we cannot find the ws then stop
            if ws == None:
                raise RuntimeError ("Cannot find workspace '" + wsName.strip() + "', aborting")
            if isinstance(ws, WorkspaceGroup):
                wsNames.extend(ws.getNames())
            else:
                wsNames.append(wsName)

        #ta = TextAxis.create(len(wsNames))
        na = NumericAxis.create(len(wsNames))
        #if (labelUsing != ""):
	#    na.title(labelUsing)
	#else:
	#    na.title("Workspaces")
	na.setUnit("TOF")
        if mtd.doesExist(wsOutput):
            DeleteWorkspace(Workspace=wsOutput)
        for wsName in wsNames:
            #extract the spectrum
            ExtractSingleSpectrum(InputWorkspace=wsName,OutputWorkspace=wsTemp,WorkspaceIndex=wsIndex)
            
            labelDouble =0.0
            if (labelUsing != ""):
                labelDouble = self.GetLogValue(mtd[wsName.strip()],labelUsing,labelValue)
            else:
                labelDouble = loopIndex + 0.0
            #ta.setValue(loopIndex,labelString)
            na.setValue(loopIndex,labelDouble)
            loopIndex += 1
            if mtd.doesExist(wsOutput):
                ConjoinWorkspaces(InputWorkspace1=wsOutput,InputWorkspace2=wsTemp,CheckOverlapping=False)
                if mtd.doesExist(wsTemp):
                    DeleteWorkspace(Workspace=wsTemp)
            else:
                RenameWorkspace(InputWorkspace=wsTemp,OutputWorkspace=wsOutput)

        wsOut = mtd[wsOutput]
        #replace the spectrun axis
        wsOut.replaceAxis(1,na)


        self.setProperty("OutputWorkspace",wsOut)
        
    def GetLogValue(self,ws,labelUsing,labelValue):
        labelDouble = 0.0
        run=ws.getRun()
        try:
            prop = run.getProperty(labelUsing)
            try:
                stats = prop.getStatistics()
                if (labelValue == "Mean"):
                    labelDouble = stats.mean
                elif (labelValue == "Median"):
                    labelDouble = stats.median
                elif (labelValue == "Maximum"):
                    labelDouble = stats.maximum
                elif (labelValue == "Minimum"):
                    labelDouble = stats.minimum
                elif (labelValue == "Last Value"):
                    labelDouble = prop.value[-1] + 0.0
                else:
                    labelDouble =  prop.value[0] + 0.0
            except:
                #this is not a time series property - just return the value
                labelDouble =  prop.value + 0.0
        except:
            #failed to find the property
            #log and pass out zero
            logger.notice("Could not find log " + labelUsing + " in workspace " + str(ws) + " using workspace label instead.")
        return labelDouble
        
AlgorithmFactory.subscribe(ConjoinSpectraNumAx)
