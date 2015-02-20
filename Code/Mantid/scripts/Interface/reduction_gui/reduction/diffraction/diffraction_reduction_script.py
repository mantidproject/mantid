"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the DgsReduction class could
    be used independently of the interface implementation
"""
import xml.dom.minidom
import os
import time
from reduction_gui.reduction.scripter import BaseReductionScripter

class DiffractionReductionScripter(BaseReductionScripter):
    """ Organizes the set of reduction parameters that will be used to
    create a reduction script. Parameters are organized by groups that
    will each have their own UI representation.

    Items in dictionary:
    1. facility_name;
    2. instrument_name
    3. _output_directory
    4. _observers
    """
    TOPLEVEL_WORKFLOWALG = "SNSPowderReductionPlus"
    WIDTH_END = "".join([" " for i in range(len(TOPLEVEL_WORKFLOWALG))])
    WIDTH = WIDTH_END + " "

    def __init__(self, name="VULCAN", facility="SNS"):
        """ Initialization
        """
        super(DiffractionReductionScripter, self).__init__(name=name, facility=facility)

        print "[diffraction_reduction_script]  Facility = %s,  Instrument = %s" % (self.facility_name, self.instrument_name)

        return

    def to_script(self, file_name=None):
        """ Generate reduction script via observers
            @param file_name: name of the file to write the script to
        """

        # 1. Collect from observers
        paramdict = {}
        for observer in self._observers:
            obstate = observer.state()
            self.parseTabSetupScript(observer._subject.__class__.__name__, obstate, paramdict)
        # ENDFOR

        # 2. Construct python commands
        script = self.constructPythonScript(paramdict)

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()

        print "[diffraction_reduction_script]  Facility = %s,  Instrument = %s" % (self.facility_name, self.instrument_name)
        print "The reduction script is ... \n", script, "\n==== End of Script ====="

        return script


    def to_xml(self, file_name=None):
        """ Extending base class to_xml
        """
        BaseReductionScripter.to_xml(self, file_name)

        return


    def parseTabSetupScript(self, tabsetuptype, setupscript, paramdict):
        """ Parse script returned from tab setup

        @param setupscript : object of SetupScript for this tab/observer
        """
        # print "ClassName: %s.  Type %s" % (tabsetuptype, type(setupscript))

        if setupscript is None:
            return

        else:
            paramdict[tabsetuptype] = {}
            terms = str(setupscript).split("\n")
            for item in terms:
                item = item.strip()
                if item == "":
                    continue

                item = item.rstrip(",")
                subterms = item.split("=", 1)
                key = subterms[0].strip()
                value = subterms[1].strip().strip("\"").strip("'")
                paramdict[tabsetuptype][key] = value
            # ENDFOR
        # ENDIF

        return

    def constructPythonScript(self, paramdict):
        """ Construct python script
        """
        # 1. Obtain all information
        runsetupdict = paramdict["RunSetupWidget"]
        advsetupdict = paramdict["AdvancedSetupWidget"]
        filterdict = paramdict["FilterSetupWidget"]

        # 2. Obtain some information
        datafilenames = self.getDataFileNames(runsetupdict, advsetupdict)
        if len(datafilenames) == 0:
            raise NotImplementedError("RunNumber cannot be neglected. ")

        dofilter = self.doFiltering(filterdict)

        # 3. Header
        script = "from mantid.simpleapi import *\n"
        script += "config['default.facility']=\"%s\"\n" % self.facility_name
        script += "\n"

        if dofilter is True:
            # a) Construct python script with generating filters
            for runtuple in datafilenames:

                runnumber = runtuple[0]
                datafilename = runtuple[1]

                # print "Working on run ", str(runnumber), " in file ", datafilename

                # i.  Load meta data only
                metadatawsname = str(datafilename.split(".")[0]+"_meta")
                splitwsname = str(datafilename.split(".")[0] + "_splitters")
                splitinfowsname = str(datafilename.split(".")[0] + "_splitinfo")

                script += "# Load data's log only\n"
                script += "Load(\n"
                script += "%sFilename = '%s',\n" % (DiffractionReductionScripter.WIDTH, datafilename)
                script += "%sOutputWorkspace = '%s',\n" % (DiffractionReductionScripter.WIDTH, metadatawsname)
                script += "%sMetaDataOnly = '1')\n" % (DiffractionReductionScripter.WIDTH)

                script += "\n"

                # ii. Generate event filters
                script += "# Construct the event filters\n"
                script += "GenerateEventsFilter(\n"
                script += "%sInputWorkspace  = '%s',\n" % (DiffractionReductionScripter.WIDTH, metadatawsname)
                script += "%sOutputWorkspace = '%s',\n" % (DiffractionReductionScripter.WIDTH, splitwsname)
                script += "%sInformationWorkspace = '%s',\n" % (DiffractionReductionScripter.WIDTH, splitinfowsname)
                if filterdict["FilterByTimeMin"] != "":
                    script += "%sStartTime = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["FilterByTimeMin"])
                if filterdict["FilterByTimeMax"] != "":
                    script += "%sStopTime  = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["FilterByTimeMax"])

                if filterdict["FilterType"] == "ByTime":
                    # Filter by time
                    script += "%sTimeInterval   = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LengthOfTimeInterval"])
                    script += "%sUnitOfTime = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["UnitOfTime"])
                    script += "%sLogName    = '%s',\n" % (DiffractionReductionScripter.WIDTH, "")

                elif filterdict["FilterType"] == "ByLogValue":
                    # Filter by log value
                    script += "%sLogName = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogName"])
                    if filterdict["MinimumLogValue"] != "":
                        script += "%sMinimumLogValue    = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["MinimumLogValue"])
                    if filterdict["MaximumLogValue"] != "":
                        script += "%sMaximumLogValue    = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["MaximumLogValue"])
                    script += "%sFilterLogValueByChangingDirection = '%s',\n" % (DiffractionReductionScripter.WIDTH,
                            filterdict["FilterLogValueByChangingDirection"])
                    if filterdict["LogValueInterval"] != "":
                        # Filter by log value interval
                        script += "%sLogValueInterval       = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogValueInterval"])
                        #if filterdict["LogName"] == "":
                        #    # No log value.  Then filter by time interval
                        #    script += "%sTimeInterval       = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogValueInterval"])
                        #else:
                        #    # Found log value interval
                        #    script += "%sLogValueInterval       = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogValueInterval"])
                    script += "%sLogBoundary    = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogBoundary"])
                    if filterdict["TimeTolerance"] != "":
                        script += "%sTimeTolerance  = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["TimeTolerance"])
                    if filterdict["LogValueTolerance"] != "":
                        script += "%sLogValueTolerance  = '%s',\n" % (DiffractionReductionScripter.WIDTH, filterdict["LogValueTolerance"])
                # ENDIF
                script += ")\n"

                # iii. Data reduction
                script += self.buildPowderDataReductionScript(runsetupdict, advsetupdict, runnumber, splitwsname, splitinfowsname)

            # ENDFOR data file names

        else:
            # b) Construct python scrpt without generating filters
            script += self.buildPowderDataReductionScript(runsetupdict, advsetupdict)

        # ENDIF : do filter

        return script


    def doFiltering(self, filterdict):
        """ Check filter dictionary to determine whether filtering is required.
        """
        dofilter = False
        if filterdict["FilterByTimeMin"] != "":
            dofilter = True
            # print "Yes! Min Generate Filter will be called!"

        if filterdict["FilterByTimeMax"] != "":
            dofilter = True
            # print "Yes! Max Generate Filter will be called!"

        if filterdict["FilterType"] != "NoFilter":
            dofilter = True
            # print "Yes! FilterType Generate Filter will be called!"

        return dofilter


    def getDataFileNames(self, runsetupdict, advsetupdict):
        """ Obtain the data file names (run names + SUFFIX)

        Return: list of files
        """
        datafilenames = []

        runnumbers_str = str(runsetupdict["RunNumber"])
        runnumbers_str = runnumbers_str.replace("\"", "")
        runnumbers_str = runnumbers_str.replace("'", "")

        # 1. Parse run numbers string to list of integers
        runnumbers = []
        terms = runnumbers_str.split(",")
        for term in terms:
            term = term.strip()
            if len(term) == 0:
                continue

            numdashes = term.count("-")
            if numdashes == 0:
                # integer
                try:
                    run = int(term)
                except ValueError:
                    print "Term %s cannot be parsed to integer.  Input error!" % (term)
                    break
                if run < 0:
                    print "Negative run number %d is not supported.  Input error!" % (run)
                    break
                runnumbers.append(run)

            elif numdashes == 1:
                # range of integer
                twovalues = term.split("-")
                try:
                    runstart = int(twovalues[0])
                    #print "run start = ", runstart
                except ValueError:
                    print "Term %s cannot be parsed to a range of integers.  Input error!" % (term)
                    break
                try:
                    runend = int(twovalues[1])
                    #print "run end = ", runend
                except ValueError:
                    print "Term %s cannot be parsed to a range of integers.  Input error!" % (term)
                    break

                for run in xrange(runstart, runend+1):
                    runnumbers.append(run)

            else:
                # cannot be correct.  more than 1 '-'
                print "Term %s cannot be parsed to an integer or a range of integers.  Input error!" % (term)
                break

            # ENDIF: number of '-'

        # ENDFOR: term

        # 2. Attach file extension
        extension = advsetupdict["Extension"].replace("\"", "").replace("'", "")
        for run in runnumbers:
            filename = str(self.instrument_name +"_" + str(run) + extension)
            datafilenames.append((run, filename))

            #print "Input data file %s of run number %s" % (filename, str(run))
        # ENDFOR

        return datafilenames


    def buildPowderDataReductionScript(self, runsetupdict, advsetupdict, runnumber=None, splitwsname=None,
                                       splitinfowsname=None):
        """ Build the script to call SNSPowderReduction()
        """
        script  = "SNSPowderReduction(\n"
        script += "%sInstrument   = '%s',\n" % (DiffractionReductionScripter.WIDTH, self.instrument_name)

        # 1. Run setup
        # a) determine whether to turn on/off corrections
        if int(runsetupdict["DisableBackgroundCorrection"]) == 1:
            runsetupdict["BackgroundNumber"] = -1
        if int(runsetupdict["DisableVanadiumCorrection"]) == 1:
            runsetupdict["VanadiumNumber"] = -1
        if int(runsetupdict["DisableVanadiumBackgroundCorrection"]) == 1:
            runsetupdict["VanadiumBackgroundNumber"] = -1

        # b) do sample X
        if int(runsetupdict["DoReSampleX"]) == 0:
            # turn off the option of SampleX
            runsetupdict["ReSampleX"] = ""

        # c) all properties
        for propname in runsetupdict.keys():
            if propname.count("Disable") == 1 and propname.count("Correction") == 1:
                # Skip disable XXXX
                continue
            if propname == "DoReSampleX":
                # Skip this
                continue

            propvalue = runsetupdict[propname]

            if propvalue == "" or propvalue is None:
                # Skip not-defined value
                continue

            if propvalue.__class__.__name__ == "bool":
                # Special treatment on boolean
                propvalue = int(propvalue)

            if propname == "RunNumber":
                # Option to take user input run number
                if runnumber is not None:
                    propvalue = str(runnumber)

            # Add value
            script += "%s%s = '%s',\n" % (DiffractionReductionScripter.WIDTH, propname, str(propvalue))
        # ENDFOR

        # 2. Advanced setup
        for propname in advsetupdict.keys():
            propvalue = advsetupdict[propname]

            if propvalue == "" or propvalue is None:
                # Skip not-defined value
                continue

            if propvalue.__class__.__name__ == "bool":
                # Special treatment on boolean
                propvalue = int(propvalue)

            # Add to script
            script += "%s%s = '%s',\n" % (DiffractionReductionScripter.WIDTH, propname, str(propvalue))
        # ENDFOR

        # 3. Optional spliter workspace
        if splitwsname is not None and splitwsname != "":
            script += "%sSplittersWorkspace = '%s',\n" % (DiffractionReductionScripter.WIDTH, str(splitwsname))
        if splitinfowsname is not None and splitinfowsname != "":
            script += "%sSplitInformationWorkspace='%s',\n" % (DiffractionReductionScripter.WIDTH,
                                                              str(splitinfowsname))
        script += "%s)\n" % (DiffractionReductionScripter.WIDTH)

        return script

    def _synInstrument(self):
        """ Syn instrument from observer-widget
        """
        # Facility instrument
        for observer in self._observers:
            observertype = observer._subject.__class__.__name__
            print "[ToScript] Observer Type = ", observertype
            if observertype.count("AdvancedWidget") == 1:
                self.instrument_name = observer._subject._instrument_name

        return

