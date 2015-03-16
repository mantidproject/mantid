#pylint: disable=invalid-name
"""
    Classes for each reduction step. Those are kept separately
    from the the interface class so that the DgsReduction class could
    be used independently of the interface implementation
"""
import os
import time
import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement

class FilterSetupScript(BaseScriptElement):
    """ Run setup script for tab 'Run Setup'
    """

    # Class static variables
    starttime = ""
    stoptime = ""
    filtertype = "NoFilter"

    filterbytime = False
    filterbylogvalue = False

    numbertimeinterval = None
    lengthtimeinterval = None
    unitoftime = "Seconds"

    logname = ""
    minimumlogvalue = ""
    maximumlogvalue = ""
    logvalueinterval = ""
    filterlogvaluebychangingdirection = "Both"
    timetolerance = ""
    logboundary = ""
    logvaluetolerance = ""
    logvaluetimesections = 1
    titleofsplitters = ""

    parnamelist = None

    def __init__(self, inst_name):
        """ Initialization
        """
        super(FilterSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """ Create a list of parameter names for SNSPowderReductionPlus()
        """
        self.parnamelist = []
        self.parnamelist.append("TitleOfSplitters")
        self.parnamelist.append("FilterByTimeMin")
        self.parnamelist.append("FilterByTimeMax")
        self.parnamelist.append("FilterType")

        self.parnamelist.append("LengthOfTimeInterval")
        self.parnamelist.append("NumberOfTimeInterval")
        self.parnamelist.append("UnitOfTime")

        self.parnamelist.append("LogName")
        self.parnamelist.append("MinimumLogValue")
        self.parnamelist.append("MaximumLogValue")
        self.parnamelist.append("LogValueInterval")
        self.parnamelist.append("FilterLogValueByChangingDirection")
        self.parnamelist.append("TimeTolerance")
        self.parnamelist.append("LogBoundary")
        self.parnamelist.append("LogValueTolerance")
        self.parnamelist.append("LogValueTimeSections")

        return

    def set_default_pars(self, inst_name):
        """ Default parameters
        """

        return


    def buildParameterDict(self):
        """ Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()
        """
        pardict = {}

        pardict["FilterByTimeMin"] = self.starttime
        pardict["FilterByTimeMax"] = self.stoptime

        pardict["NumberOfTimeInterval"] = self.numbertimeinterval
        pardict["LengthOfTimeInterval"] = self.lengthtimeinterval
        pardict["UnitOfTime"] = self.unitoftime

        pardict["LogName"] = self.logname
        pardict["MinimumLogValue"] = self.minimumlogvalue
        pardict["MaximumLogValue"] = self.maximumlogvalue
        pardict["LogValueInterval"] = self.logvalueinterval
        pardict["FilterLogValueByChangingDirection"] = self.filterlogvaluebychangingdirection
        pardict["TimeTolerance"] = self.timetolerance
        pardict["LogBoundary"] = self.logboundary
        pardict["LogValueTolerance"] = self.logvaluetolerance
        pardict["LogValueTimeSections"] = self.logvaluetimesections
        pardict["TitleOfSplitters"] = self.titleofsplitters

        # 2. Sort out some issue
        filtertype = "NoFilter"
        if self.filterbytime is True and self.filterbylogvalue is True:
            print "Impossible situation!  Coding must be wrong!"
        elif self.filterbytime is True:
            filtertype = "ByTime"
        elif self.filterbylogvalue is True:
            filtertype = "ByLogValue"
        pardict["FilterType"] = filtertype

        return pardict

    def to_script(self):
        """ 'Public'  method  to save the current GUI to string via str() and general class ReductionScript
        """
        # 1. Output values
        parnamevaluedict = self.buildParameterDict()

        # 2. Add to script
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            script += "%-10s = %s\n" % (parname, parvalue)
            # if parvalue != "":
            # script += "%-10s = \"%s\"\n" % (parname, parvalue)
            # if parname != self.parnamelist[-1]:
            #    script += ",\n"
        #ENDFOR

        return script

    def to_xml(self):
        """ 'Public' method to create XML from the current data.
        """
        pardict = self.buildParameterDict()

        xml = "<FilterSetup>\n"
        for parname in self.parnamelist:
            keyname = parname.lower()
            parvalue = pardict[parname]
            xml += "  <%s>%s</%s>\n" % (keyname, str(parvalue), keyname)
        xml += "</FilterSetup>\n"

        return xml

    def from_xml(self, xml_str):
        """ 'Public' method to read in data from XML
            @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("FilterSetup")
        if len(element_list)>0:
            instrument_dom = element_list[0]

            self.titleofsplitters = self.getStringElement(instrument_dom, "titleofsplitters", FilterSetupScript.titleofsplitters)

            self.starttime = self.getFloatElement(instrument_dom, "filterbytimemin", FilterSetupScript.starttime)
            self.stoptime = self.getFloatElement(instrument_dom, "filterbytimemax", FilterSetupScript.stoptime)

            filtertype = self.getStringElement(instrument_dom, "filtertype", FilterSetupScript.filtertype)

            self.numbertimeinterval = self.getIntegerElement(instrument_dom, "numberoftimeinterval", FilterSetupScript.numbertimeinterval)
            self.lengthtimeinterval = self.getFloatElement(instrument_dom, "lengthoftimeinterval", FilterSetupScript.lengthtimeinterval)
            self.unitoftime = self.getStringElement(instrument_dom, "unitoftime", FilterSetupScript.unitoftime)

            self.logname = self.getStringElement(instrument_dom, "logname", FilterSetupScript.logname)
            self.logvaluetolerance = self.getFloatElement(instrument_dom, "logvaluetolerance", FilterSetupScript.logvaluetolerance)
            self.minimumlogvalue = self.getFloatElement(instrument_dom, "minimumlogvalue", FilterSetupScript.minimumlogvalue)
            self.maximumlogvalue = self.getFloatElement(instrument_dom, "maximumlogvalue", FilterSetupScript.maximumlogvalue)
            self.logvalueinterval = self.getFloatElement(instrument_dom, "logvalueinterval", FilterSetupScript.logvalueinterval)
            self.filterlogvaluebychangingdirection = self.getStringElement(instrument_dom, "filterlogvaluebychangingdirection",\
                    FilterSetupScript.filterlogvaluebychangingdirection)
            self.timetolerance = self.getFloatElement(instrument_dom, "timetolerance", FilterSetupScript.timetolerance)
            self.logboundary = self.getStringElement(instrument_dom, "logboundary", FilterSetupScript.logboundary)

            self.logvaluetimesections = 1

            if filtertype == "ByTime":
                self.filterbytime = True
                self.filterbylogvalue = False
            elif filtertype == "ByLogValue":
                self.filterbytime = False
                self.filterbylogvalue = True
            else:
                self.filterbytime = False
                self.filterbylogvalue = False

            return

    def reset(self):
        """ 'Public' method to reset state
        """
        self.starttime           = FilterSetupScript.starttime
        self.stoptime            = FilterSetupScript.stoptime
        self.filterbytime        = FilterSetupScript.filterbytime
        self.filterbylogvalue    = FilterSetupScript.filterbylogvalue
        self.numbertimeinterval  = FilterSetupScript.numbertimeinterval
        self.lengthtimeinterval  = FilterSetupScript.lengthtimeinterval
        self.unitoftime          = FilterSetupScript.unitoftime
        self.logname             = FilterSetupScript.logname
        self.minimumlogvalue     = FilterSetupScript.minimumlogvalue
        self.maximumlogvalue     = FilterSetupScript.maximumlogvalue
        self.logvalueinterval    = FilterSetupScript.logvalueinterval
        self.timetolerance       = FilterSetupScript.timetolerance
        self.logboundary         = FilterSetupScript.logboundary
        self.logvaluetolerance   = FilterSetupScript.logvaluetolerance
        self.titleofsplitters    = FilterSetupScript.titleofsplitters
        self.logvaluetimesections     = FilterSetupScript.logvaluetimesections
        self.filterlogvaluebychangingdirection = FilterSetupScript.filterlogvaluebychangingdirection

        return

    def getFloatElement(self, instrument_dom, xmlname, default):
        """ Get a float value from xml
        """
        floatstr = BaseScriptElement.getStringElement(instrument_dom, xmlname, default)

        if floatstr != "" and floatstr is not None and floatstr != "None":
            try:
                value = float(floatstr)
            except ValueError:
                print "Warning! XML field %s value %s cannot be converted to float" % (xmlname, floatstr)
                value = None
        else:
            value = None

        return value

    def getIntegerElement(self, instrument_dom, xmlname, default):
        """ Get a float value from xml
        """
        integerstr = BaseScriptElement.getStringElement(instrument_dom, xmlname, default)
        if integerstr != "" and integerstr is not None and integerstr != "None":
            try:
                value = int(integerstr)
            except ValueError:
                print "Warning! XML field %s value %s cannot be converted to integer" % (xmlname, integerstr)
                value = None
        else:
            value = None

        return value

    def getStringElement(self, instrument_dom, xmlname, default):
        """ Get a float value from xml
        """
        value = BaseScriptElement.getStringElement(instrument_dom, xmlname, default)

        return value

