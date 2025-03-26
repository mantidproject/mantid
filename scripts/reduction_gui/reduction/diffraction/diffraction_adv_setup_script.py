# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
"""
Classes for each reduction step. Those are kept separately
from the interface class so that the DgsReduction class could
be used independently of the interface implementation
"""

import xml.dom.minidom

from reduction_gui.reduction.scripter import BaseScriptElement


def getBooleanElement(instrument_dom, keyname, default):
    """Get a boolean value from an element.
    Boolean can be recorded as
    (1) True/False
    (2) 1/0
    """
    tempbool = BaseScriptElement.getStringElement(instrument_dom, keyname, default=default)

    if tempbool == "True":
        tempbool = 1
    elif tempbool == "False":
        tempbool = 0

    return bool(int(tempbool))


def getFloatElement(instrument_dom, keyname, default):
    """Get a float from the xml document. Conversion errors
    return the default value.
    """
    try:
        return BaseScriptElement.getFloatElement(instrument_dom, keyname, default=default)
    except ValueError:
        return default


class AdvancedSetupScript(BaseScriptElement):
    """Run setup script for tab 'Run Setup'"""

    #
    # Class attributes
    #
    pushdatapositive = "None"
    offsetdata = 0.0
    lowresref = ""
    cropwavelengthmin = ""
    cropwavelengthmax = ""
    removepropmppulsewidth = 50.0
    maxchunksize = ""
    filterbadpulses = 95.0
    bkgdsmoothpars = ""
    stripvanadiumpeaks = True
    vanadiumfwhm = ""
    vanadiumpeaktol = ""
    vanadiumsmoothparams = ""
    vanadiumradius = 0.3175
    preserveevents = True
    extension = "_event.nxs"
    outputfileprefix = ""
    scaledata = ""
    sampleformula = ""
    samplenumberdensity = ""
    measuredmassdensity = ""
    samplegeometry = {}
    containershape = ""
    typeofcorrection = ""
    parnamelist = None
    # Caching options
    cache_dir_scan_save = ""  # Cache search candidate 1
    cache_dir_scan_1 = ""  # Cache search candidate 2
    cache_dir_scan_2 = ""  # Cache search candidate 3
    clean_cache = False  # determines whether to delete all cache files within the cache directory

    @property
    def cache_dir(self):
        """Passing all three candidates back as one list"""
        # A comma ',' is used here since it is the default delimiter in mantid
        return ",".join((self.cache_dir_scan_save, self.cache_dir_scan_1, self.cache_dir_scan_2))

    def __init__(self, inst_name):
        """Initialization"""
        super(AdvancedSetupScript, self).__init__()
        self.createParametersList()

        self.set_default_pars(inst_name)
        self.reset()

        return

    def createParametersList(self):
        """Create a list of parameter names for SNSPowderReductionPlus()"""
        self.parnamelist = [
            "LowResRef",
            "CropWavelengthMin",
            "CropWavelengthMax",
            "RemovePromptPulseWidth",
            "MaxChunkSize",
            "StripVanadiumPeaks",
            "VanadiumFWHM",
            "VanadiumPeakTol",
            "VanadiumSmoothParams",
            "VanadiumRadius",
            "FilterBadPulses",
            "BackgroundSmoothParams",
            "PushDataPositive",
            "OffsetData",
            "PreserveEvents",
            "OutputFilePrefix",
            "ScaleData",
            "TypeOfCorrection",
            "SampleFormula",
            "MeasuredMassDensity",
            "SampleGeometry",
            "SampleNumberDensity",
            "ContainerShape",
            # Caching options
            "CacheDir",
            "CleanCache",
        ]

    def set_default_pars(self, inst_name):
        """Set default values"""

        return

    def to_script(self):
        """'Public'  method  to save the current GUI to string via str() and general class ReductionScript"""
        # 1. Form (partial) script
        parnamevaluedict = self.buildParameterDict()
        script = ""
        for parname in self.parnamelist:
            parvalue = parnamevaluedict[parname]
            if parvalue != "" and parname != "Instrument" and parname != "Facility":
                if str(parvalue) == "True":
                    parvalue = "1"
                elif str(parvalue) == "False":
                    parvalue = "0"
                if not isinstance(parvalue, dict):
                    script += '%-10s = "%s",\n' % (parname, parvalue)
                else:
                    script += "%-10s = %s,\n" % (parname, parvalue)
        # ENDFOR

        return script

    def buildParameterDict(self):
        """Create a dictionary for parameter and parameter values for SNSPowderReductionPlus()"""
        pardict = {}

        pardict["LowResRef"] = self.lowresref
        pardict["CropWavelengthMin"] = self.cropwavelengthmin
        pardict["CropWavelengthMax"] = self.cropwavelengthmax
        pardict["RemovePromptPulseWidth"] = self.removepropmppulsewidth
        pardict["MaxChunkSize"] = self.maxchunksize
        pardict["FilterBadPulses"] = self.filterbadpulses
        pardict["BackgroundSmoothParams"] = self.bkgdsmoothpars
        # these two parameters cannot be specified at the same time so
        # initally set them at their default values
        pardict["PushDataPositive"] = "None"
        pardict["OffsetData"] = 0.0
        if self.pushdatapositive == "None":
            pardict["OffsetData"] = self.offsetdata
        else:
            pardict["PushDataPositive"] = self.pushdatapositive

        pardict["StripVanadiumPeaks"] = self.stripvanadiumpeaks
        pardict["VanadiumFWHM"] = self.vanadiumfwhm
        pardict["VanadiumPeakTol"] = self.vanadiumpeaktol
        pardict["VanadiumSmoothParams"] = self.vanadiumsmoothparams
        pardict["VanadiumRadius"] = self.vanadiumradius
        pardict["PreserveEvents"] = str(int(self.preserveevents))
        pardict["OutputFilePrefix"] = self.outputfileprefix
        pardict["ScaleData"] = self.scaledata
        pardict["TypeOfCorrection"] = self.typeofcorrection
        pardict["SampleFormula"] = self.sampleformula
        pardict["MeasuredMassDensity"] = self.measuredmassdensity
        pardict["SampleGeometry"] = self.samplegeometry
        pardict["SampleNumberDensity"] = self.samplenumberdensity
        pardict["ContainerShape"] = self.containershape
        # Caching options
        pardict["CacheDir"] = self.cache_dir
        pardict["CleanCache"] = str(int(self.clean_cache))

        return pardict

    def to_xml(self):
        """'Public' method to create XML from the current data."""

        xml = "<AdvancedSetup>\n"
        for keyname, value in self.buildParameterDict().items():
            # casting value to string
            if isinstance(value, bool):
                # special map for bool type
                value = "1" if value else "0"
            else:
                value = str(value)

            xml += f"<{keyname.lower()}>{value}</{keyname.lower()}>\n"
        xml += "</AdvancedSetup>\n"

        return xml

    def from_xml(self, xml_str):
        """'Public' method to read in data from XML
        @param xml_str: text to read the data from
        """
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("AdvancedSetup")
        if len(element_list) > 0:
            instrument_dom = element_list[0]

            self.lowresref = getFloatElement(instrument_dom, "lowresref", AdvancedSetupScript.lowresref)

            self.cropwavelengthmin = getFloatElement(instrument_dom, "cropwavelengthmin", AdvancedSetupScript.cropwavelengthmin)

            self.cropwavelengthmax = getFloatElement(instrument_dom, "cropwavelengthmax", AdvancedSetupScript.cropwavelengthmax)

            self.removepropmppulsewidth = getFloatElement(
                instrument_dom, "removepromptpulsewidth", AdvancedSetupScript.removepropmppulsewidth
            )

            try:
                self.maxchunksize = BaseScriptElement.getIntElement(
                    instrument_dom, "maxchunksize", default=AdvancedSetupScript.maxchunksize
                )
            except ValueError:
                self.maxchunksize = AdvancedSetupScript.maxchunksize

            self.filterbadpulses = getFloatElement(instrument_dom, "filterbadpulses", AdvancedSetupScript.filterbadpulses)

            self.bkgdsmoothpars = BaseScriptElement.getStringElement(
                instrument_dom, "backgroundsmoothparams", default=AdvancedSetupScript.bkgdsmoothpars
            )

            self.pushdatapositive = BaseScriptElement.getStringElement(
                instrument_dom, "pushdatapositive", default=AdvancedSetupScript.pushdatapositive
            )
            self.offsetdata = BaseScriptElement.getStringElement(instrument_dom, "offsetdata", default=AdvancedSetupScript.offsetdata)

            self.stripvanadiumpeaks = getBooleanElement(instrument_dom, "stripvanadiumpeaks", AdvancedSetupScript.stripvanadiumpeaks)

            self.vanadiumfwhm = getFloatElement(instrument_dom, "vanadiumfwhm", AdvancedSetupScript.vanadiumfwhm)

            self.vanadiumpeaktol = getFloatElement(instrument_dom, "vanadiumpeaktol", AdvancedSetupScript.vanadiumpeaktol)

            self.vanadiumsmoothparams = BaseScriptElement.getStringElement(
                instrument_dom, "vanadiumsmoothparams", default=AdvancedSetupScript.vanadiumsmoothparams
            )

            self.vanadiumradius = getFloatElement(instrument_dom, "vanadiumradius", AdvancedSetupScript.vanadiumradius)

            self.extension = BaseScriptElement.getStringElement(instrument_dom, "extension", default=AdvancedSetupScript.extension)

            self.preserveevents = getBooleanElement(instrument_dom, "preserveevents", default=AdvancedSetupScript.preserveevents)

            self.outputfileprefix = BaseScriptElement.getStringElement(
                instrument_dom, "outputfileprefix", default=AdvancedSetupScript.outputfileprefix
            )

            self.scaledata = getFloatElement(instrument_dom, "scaledata", AdvancedSetupScript.scaledata)

            self.sampleformula = BaseScriptElement.getStringElement(
                instrument_dom, "sampleformula", default=AdvancedSetupScript.sampleformula
            )

            self.samplenumberdensity = getFloatElement(instrument_dom, "samplenumberdensity", AdvancedSetupScript.samplenumberdensity)

            self.measuredmassdensity = getFloatElement(instrument_dom, "measuredmassdensity", AdvancedSetupScript.measuredmassdensity)

            string_tmp = BaseScriptElement.getStringElement(instrument_dom, "samplegeometry", default=AdvancedSetupScript.samplegeometry)
            self.samplegeometry = {}
            if string_tmp:
                items = string_tmp.replace("{", "").replace("}", "").split(",")
                keys_tmp = [item.split(":")[0].replace("'", "") for item in items]
                vals_tmp = [item.split(":")[1].replace("'", "").strip() for item in items]
                if vals_tmp.count("") == 0:
                    vals_tmp = [float(item) for item in vals_tmp]
                    for count, value in enumerate(keys_tmp):
                        self.samplegeometry[value] = vals_tmp[count]

            self.containershape = BaseScriptElement.getStringElement(
                instrument_dom, "containershape", default=AdvancedSetupScript.containershape
            )

            self.typeofcorrection = BaseScriptElement.getStringElement(
                instrument_dom, "typeofcorrection", default=AdvancedSetupScript.typeofcorrection
            )

            # Caching options
            # split it into the three cache dirs
            # NOTE: there should only be three entries, if not, let it fail early
            try:
                self.cache_dir_scan_save, self.cache_dir_scan_1, self.cache_dir_scan_2 = (
                    BaseScriptElement.getStringElement(instrument_dom, "cachedir", default=";;").replace(";", ",").split(",")
                )
            except ValueError:
                self.cache_dir_scan_save = ""
                self.cache_dir_scan_1 = ""
                self.cache_dir_scan_2 = ""

            tempbool = BaseScriptElement.getStringElement(instrument_dom, "cleancache", default=str(int(self.__class__.clean_cache)))
            self.clean_cache = bool(int(tempbool))

    def reset(self):
        r"""reset instance's attributes with the values of the class' attributes"""
        class_attrs_selected = [
            "pushdatapositive",
            "offsetdata",
            "lowresref",
            "cropwavelengthmin",
            "cropwavelengthmax",
            "removepropmppulsewidth",
            "maxchunksize",
            "filterbadpulses",
            "bkgdsmoothpars",
            "stripvanadiumpeaks",
            "vanadiumfwhm",
            "vanadiumpeaktol",
            "vanadiumsmoothparams",
            "preserveevents",
            "extension",
            "outputfileprefix",
            # Caching options
            "cache_dir_scan_save",
            "cache_dir_scan_1",
            "cache_dir_scan_2",
            "clean_cache",
        ]
        [setattr(self, attr, getattr(self.__class__, attr)) for attr in class_attrs_selected]
        return
