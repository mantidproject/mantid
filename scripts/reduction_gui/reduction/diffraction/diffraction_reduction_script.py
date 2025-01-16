# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,R0912
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the DgsReduction class could
be used independently of the interface implementation
"""

import os
from mantid.kernel import Logger
from mantid.api import FileFinder
from reduction_gui.reduction.scripter import BaseReductionScripter


class DiffractionReductionScripter(BaseReductionScripter):
    """Organizes the set of reduction parameters that will be used to
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
    AUTOSCRIPTNAME = "SNSPowderReductionScript_AutoSave.py"

    def __init__(self, name, facility="SNS"):
        """Initialization"""
        # Call base class
        super(DiffractionReductionScripter, self).__init__(name=name, facility=facility)

        # Find whether there is stored setup XMLs
        homedir = os.path.expanduser("~")
        mantidconfigdir = os.path.join(homedir, ".mantid")
        self.configDir = mantidconfigdir

        # create configuration dir if it has not been
        if os.path.exists(self.configDir) is False:
            os.makedirs(self.configDir)

        # Information output
        if self.facility_name is False:
            self.facility_name = "SNS"
        dbmsg = "[SNS Powder Reduction]  Facility = %s,  Instrument = %s\nAuto-save Directory %s" % (
            self.facility_name,
            self.instrument_name,
            mantidconfigdir,
        )
        Logger("DiffractionReductionScripter").debug(str(dbmsg))

        return

    def to_script(self, file_name=None):
        """Generate reduction script via observers and
        (1) save the script to disk and (2) save the reduction setup to disk.

        Arguments:
         - file_name: name of the file to write the script to
        """
        # Collect partial scripters from observers
        paramdict = {}
        for observer in self._observers:
            obstate = observer.state()
            self.parseTabSetupScript(observer._subject.__class__.__name__, obstate, paramdict)
        # ENDFOR

        # Construct python commands
        script = self.constructPythonScript(paramdict)

        # Save script to disk
        if file_name is None:
            file_name = os.path.join(self.configDir, DiffractionReductionScripter.AUTOSCRIPTNAME)

        try:
            f = open(file_name, "w")
            f.write(script)
            f.close()
        except IOError as e:
            print("Unable to save script to file. Reason: %s." % (str(e)))

        # Export XML file
        autosavexmlfname = os.path.join(self.configDir, "snspowderreduction.xml")
        self.to_xml(autosavexmlfname)

        # Information output
        wbuf = "Reduction script: (script is saved to %s; setup is saved to %s. \n" % (file_name, autosavexmlfname)
        wbuf += script
        wbuf += "\n========== End of Script ==========="
        print(wbuf)

        return script

    def to_xml(self, file_name=None):
        """Extending base class to_xml"""
        BaseReductionScripter.to_xml(self, file_name)

        return

    def parseTabSetupScript(self, tabsetuptype, setupscript, paramdict):
        """Parse script returned from tab setup

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
                value = subterms[1].strip().strip('"').strip("'")
                paramdict[tabsetuptype][key] = value
            # ENDFOR
        # ENDIF

        return

    def constructPythonScript(self, paramdict):
        """Construct python script"""
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

        if dofilter:
            # a) Construct python script with generating filters
            for runtuple in datafilenames:
                runnumber = runtuple[0]
                datafilename = runtuple[1]

                # print "Working on run ", str(runnumber), " in file ", datafilename

                # i.  Load meta data only
                metadatawsname = str(datafilename.split(".")[0] + "_meta")
                splitwsname = str(datafilename.split(".")[0] + "_splitters")
                splitinfowsname = str(datafilename.split(".")[0] + "_splitinfo")

                script += "# Load data's log only\n"
                script += "Load(\n"
                script += "{}Filename = '{}',\n".format(DiffractionReductionScripter.WIDTH, datafilename)
                script += "{}OutputWorkspace = '{}',\n".format(DiffractionReductionScripter.WIDTH, metadatawsname)
                script += "{}MetaDataOnly = True)\n".format(DiffractionReductionScripter.WIDTH)

                script += "\n"

                # ii. Generate event filters
                script += "# Construct the event filters\n"
                script += "GenerateEventsFilter(\n"
                script += "{}InputWorkspace  = '{}',\n".format(DiffractionReductionScripter.WIDTH, metadatawsname)
                script += "{}OutputWorkspace = '{}',\n".format(DiffractionReductionScripter.WIDTH, splitwsname)
                script += "{}InformationWorkspace = '{}',\n".format(DiffractionReductionScripter.WIDTH, splitinfowsname)
                if filterdict["FilterByTimeMin"] != "":
                    script += "{}StartTime = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["FilterByTimeMin"])
                if filterdict["FilterByTimeMax"] != "":
                    script += "{}StopTime  = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["FilterByTimeMax"])

                if filterdict["FilterType"] == "ByTime":
                    # Filter by time
                    script += "{}TimeInterval   = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["LengthOfTimeInterval"])
                    script += "{}UnitOfTime = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["UnitOfTime"])
                    script += "{}LogName    = '',\n".format(DiffractionReductionScripter.WIDTH)  # intentionally empty

                elif filterdict["FilterType"] == "ByLogValue":
                    # Filter by log value
                    script += "{}LogName = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["LogName"])
                    if filterdict["MinimumLogValue"] != "":
                        script += "{}MinimumLogValue    = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["MinimumLogValue"])
                    if filterdict["MaximumLogValue"] != "":
                        script += "{}MaximumLogValue    = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["MaximumLogValue"])
                    script += "{}FilterLogValueByChangingDirection = '{}',\n".format(
                        DiffractionReductionScripter.WIDTH, filterdict["FilterLogValueByChangingDirection"]
                    )
                    if filterdict["LogValueInterval"] != "":
                        # Filter by log value interval
                        script += "{}LogValueInterval       = '{}',\n".format(
                            DiffractionReductionScripter.WIDTH, filterdict["LogValueInterval"]
                        )
                    script += "{}LogBoundary    = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["LogBoundary"])
                    if filterdict["TimeTolerance"] != "":
                        script += "{}TimeTolerance  = '{}',\n".format(DiffractionReductionScripter.WIDTH, filterdict["TimeTolerance"])
                    if filterdict["LogValueTolerance"] != "":
                        script += "{}LogValueTolerance  = '{}',\n".format(
                            DiffractionReductionScripter.WIDTH, filterdict["LogValueTolerance"]
                        )
                # ENDIF
                script += ")\n"

                # iii. Data reduction
                script += self.buildPowderDataReductionScript(runsetupdict, advsetupdict, runnumber, splitwsname, splitinfowsname)

            # ENDFOR data file names

        else:
            # b) Construct python script without generating filters
            script += self.buildPowderDataReductionScript(runsetupdict, advsetupdict)

        # ENDIF : do filter

        print("Script and Save XML to default.")

        return script

    def doFiltering(self, filterdict):
        """Check filter dictionary to determine whether filtering is required."""
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
        """Obtain the data file names (run names + SUFFIX)

        Return: list of files
        """

        runnumbers_str = str(runsetupdict["RunNumber"])
        if runnumbers_str.count(":") > 0:
            runnumbers_str = runnumbers_str.replace(":", "-")
        runnumbers_str = FileFinder.findRuns("{}{}".format(self.instrument_name, runnumbers_str))
        runnumbers_str = [os.path.split(filename)[-1] for filename in runnumbers_str]

        # create an integer version
        runnumbers = []
        for filename in runnumbers_str:
            for extension in ["_event.nxs", ".nxs.h5"]:
                filename = filename.replace(extension, "")
            runnumber = filename.split("_")[-1]
            runnumbers.append(int(runnumber))

        # put together the output
        datafilenames = []
        for filename, runnumber in zip(runnumbers_str, runnumbers):
            datafilenames.append((runnumber, filename))

        return datafilenames

    def buildPowderDataReductionScript(self, runsetupdict, advsetupdict, runnumber=None, splitwsname=None, splitinfowsname=None):
        """Build the script to call SNSPowderReduction()"""
        script = "SNSPowderReduction(\n"

        # 1. Run setup
        # a) determine whether to turn on/off corrections
        if int(runsetupdict["DisableBackgroundCorrection"]) == 1:
            runsetupdict["BackgroundNumber"] = -1
        if int(runsetupdict["DisableVanadiumCorrection"]) == 1:
            runsetupdict["VanadiumNumber"] = -1
        if int(runsetupdict["DisableVanadiumBackgroundCorrection"]) == 1:
            runsetupdict["VanadiumBackgroundNumber"] = -1
        if int(runsetupdict["EnableInterpolate"]) == 0:
            runsetupdict["InterpolateTargetTemp"] = ""

        # b) do resample X or binning
        if int(runsetupdict["DoReSampleX"]) == 0:
            # turn off the option of SampleX
            runsetupdict["ResampleX"] = ""
        else:
            # turn off the binning
            runsetupdict["Binning"] = ""

        # only NOMAD uses 'ExpIniFile'
        if not self.instrument_name.lower().startswith("nom"):
            runsetupdict.pop("ExpIniFile", None)

        # c) all properties
        for propname, propvalue in runsetupdict.items():
            # skip these pseudo-properties
            if propname in [
                "DisableBackgroundCorrection",
                "DisableVanadiumCorrection",
                "DisableVanadiumBackgroundCorrection",
                "DoReSampleX",
                "EnableInterpolate",
            ]:
                continue

            if propvalue == "" or propvalue is None:
                # Skip not-defined value
                continue

            if propname == "RunNumber":
                propname = "Filename"  # change to what SNSPowderReduction uses

                # option to take user input run number
                if runnumber is not None:
                    propvalue = runnumber

                # add the instrument name to the file hint
                propvalue = "{}_{}".format(self.instrument_name, str(propvalue))

            # Add value
            script += "{}{} = '{}',\n".format(DiffractionReductionScripter.WIDTH, propname, propvalue)
        # ENDFOR

        # 2. Advanced setup
        for propname, propvalue in advsetupdict.items():
            if propvalue == "" or propvalue is None:
                # Skip not-defined value
                continue

            # Add to script
            if "{" in propvalue and "}" in propvalue:
                script += "{}{} = {},\n".format(DiffractionReductionScripter.WIDTH, propname, propvalue)
            else:
                script += "{}{} = '{}',\n".format(DiffractionReductionScripter.WIDTH, propname, propvalue)
        # ENDFOR

        # 3. Optional spliter workspace
        if splitwsname is not None and splitwsname != "":
            script += "{}SplittersWorkspace = '{}',\n".format(DiffractionReductionScripter.WIDTH, str(splitwsname))
        if splitinfowsname is not None and splitinfowsname != "":
            script += "{}SplitInformationWorkspace='{}',\n".format(DiffractionReductionScripter.WIDTH, str(splitinfowsname))
        script += "{})\n".format(DiffractionReductionScripter.WIDTH)

        return script

    def _synInstrument(self):
        """Syn instrument from observer-widget"""
        # Facility instrument
        for observer in self._observers:
            observertype = observer._subject.__class__.__name__
            print("[ToScript] Observer Type = ", observertype)
            if observertype.count("AdvancedWidget") == 1:
                self.instrument_name = observer._subject._instrument_name

        return
