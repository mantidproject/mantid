#pylint: disable=invalid-name
"""
    Reduction scripter used to take reduction parameters
    end produce a Mantid reduction script
"""
# Check whether Mantid is available
try:
    from mantid.kernel import ConfigService, Logger, version_str
    HAS_MANTID = True
except:
    HAS_MANTID = False

try:
    import mantidplot
    HAS_MANTIDPLOT = True
except:
    HAS_MANTIDPLOT = False

import xml.dom.minidom
import sys
import time
import platform
import re
import os

class BaseScriptElement(object):
    """
        Base class for each script element (panel on the UI).
        Contains only data and is UI implementation agnostic.
    """

    UPDATE_1_CHANGESET_CUTOFF = 10735

    def __str__(self):
        """
            Script representation of the object.
            The output is meant to be executable as a Mantid python script
        """
        return self.to_script()

    def to_script(self):
        """
            Generate reduction script
        """
        return ""

    def update(self):
        """
            Update data member after the reduction has been executed
        """
        return NotImplemented

    def apply(self):
        """
            Method called to apply the reduction script element
            to a Mantid Reducer
        """
        return NotImplemented

    def to_xml(self):
        """
            Return an XML representation of the data / state of the object
        """
        return ""

    def from_xml(self, xml_str):
        """
            Parse the input text as XML to populate the data members
            of this object
        """
        return NotImplemented

    def reset(self):
        """
            Reset the state to default
        """
        return NotImplemented

    @classmethod
    def parse_runs(cls, range_str):
        """
            Return a list of runs. Parses for run ranges indicated
            by dashes.
            @param range_str: string representing a range of runs
        """
        range_str = range_str.strip()
        # Parse for dash
        toks = range_str.split('-')
        # If we have two sub-strings separated by a dash, treat
        # it as a range if the two sub-strings can be converted
        # to integers
        if len(toks)==2:
            try:
                r_min = int(toks[0])
                r_max = int(toks[1])
                if r_max>r_min:
                    return [str(i) for i in range(r_min, r_max+1)]
            except:
                # Can't convert to run numbers, just skip
                pass
        return [range_str]

    @classmethod
    def getText(cls, nodelist):
        """
            Utility method to extract text out of an XML node
        """
        rc = ""
        for node in nodelist:
            if node.nodeType == node.TEXT_NODE:
                rc = rc + node.data
        return rc

    @classmethod
    def getContent(cls, dom, tag):
        element_list = dom.getElementsByTagName(tag)
        if len(element_list)>0:
            return BaseScriptElement.getText(element_list[0].childNodes)
        else:
            return None

    @classmethod
    def getIntElement(cls, dom, tag, default=None):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return int(value)
        else:
            return default

    @classmethod
    def getIntList(cls, dom, tag, default=[]):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None and len(value.strip())>0:
            return map(int, value.split(','))
        else:
            return default

    @classmethod
    def getFloatElement(cls, dom, tag, default=None):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return float(value)
        else:
            return default

    @classmethod
    def getFloatList(cls, dom, tag, default=[]):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None and len(value.strip())>0:
            return map(float, value.split(','))
        else:
            return default

    @classmethod
    def getStringElement(cls, dom, tag, default=''):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value
        else:
            return default

    @classmethod
    def getStringList(cls, dom, tag, default=[]):
        elem_list = []
        element_list = dom.getElementsByTagName(tag)
        if len(element_list)>0:
            for l in element_list:
                elem_list.append(BaseScriptElement.getText(l.childNodes).strip())
        return elem_list

    @classmethod
    def getBoolElement(cls, dom, tag, true_tag='true', default=False):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value.lower()==true_tag.lower()
        else:
            return default

    @classmethod
    def getMantidBuildVersion(cls, dom):
        """
            Get the mantid commit number. The format of the build version
            is <release>.<sub-version>.<commit number>
        """
        element_list = dom.getElementsByTagName("mantid_version")
        if len(element_list)>0:
            version_str = BaseScriptElement.getText(element_list[0].childNodes)
            version_list = version_str.split('.')
            if len(version_list)==3:
                change_set = int(version_list[2])
                return change_set
        return -1

    @classmethod
    def getPropertyValue(cls, algorithm, property_name, default=None):
        """
            Look up the property of a Mantid algorithm object and return
            its value or the specified default
            @param algorithm: Mantid algorithm object
            @param property_name: name of the property
            @param default: default value to return
        """
        if algorithm.existsProperty(property_name):
            prop = algorithm.getProperty(property_name)
            # If we have a number and it's very close to the maximum value we
            # can handle, treat is as an EMPTY_DBL
            if prop.type=='number' and prop.value>sys.float_info.max/10.0:
                return default
            return prop.value
        return default

    @classmethod
    def addElementToSection(cls, xml_str, parent_name, tag, content=None):
        """
            Adds a child element to a given parent element of an XML string.
            @param xml_str: string representation of an XML block [str]
            @param parent_name: parent element to add the child under [str]
            @param tag: element name [str]
            @param content: content of the new child element [str]
        """
        # Encapsulate the xml in case the input is not properly nested
        xml_str = "<tmp>%s</tmp>" % xml_str
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName(parent_name)
        if len(element_list)>0:
            instrument_dom = element_list[0]
            child = dom.createElement(tag)
            if content is not None:
                content_node = dom.createTextNode(content)
                child.appendChild(content_node)
            instrument_dom.appendChild(child)

        # Loop over the child elements on tmp in case there is more than one
        # Clean up the resulting XML string since toprettyxml() is not pretty enough
        output_str = ""
        for item in dom.getElementsByTagName("tmp")[0].childNodes:
            uglyxml = item.toprettyxml(indent='  ', newl='\n')
            text_re = re.compile('>\n\s+([^<>\s].*?)\n\s+</', re.DOTALL)
            prettierxml = text_re.sub('>\g<1></', uglyxml)

            text_re = re.compile('((?:^)|(?:</[^<>\s].*?>)|(?:<[^<>\s].*?>)|(?:<[^<>\s].*?/>))\s*\n+', re.DOTALL)
            output_str += text_re.sub('\g<1>\n', prettierxml)
        return output_str

class BaseReductionScripter(object):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    ## List of observers
    _observers = []

    class ReductionObserver(object):

        ## Script element class (for type checking)
        _state_cls = None
        ## Script element object
        _state = None
        ## Observed widget object
        _subject = None

        def __init__(self, subject):
            self._subject = subject
            self.update(True)

        def update(self, init=False):
            """
                Retrieve state from observed widget
                @param init: if True, the state class will be kept for later type checking
            """

            self._state = self._subject.get_state()

            # If we are initializing, store the object class
            if init:
                self._state_cls = self._state.__class__

            # check that the object class is consistent with what was initially stored
            elif not self._state.__class__ == self._state_cls:
                raise RuntimeError, "State class changed at runtime, was %s, now %s" % (self._state_cls, self._state.__class__)

        def push(self):
            """
                Push the state to update the observed widget
            """
            self._subject.set_state(self.state())

        def state(self):
            """
                Returns the state, if one is defined.
            """
            if self._state == NotImplemented:
                return None
            elif self._state is None:
                raise RuntimeError, "Error with %s widget: state not initialized" % self._subject.__class__
            return self._state

        def reset(self):
            """
                Reset state
            """
            if self._state is not None and self._state is not NotImplemented:
                self._state.reset()
            else:
                raise RuntimeError, "State reset called without a valid initialized state"


    def __init__(self, name="", facility=""):
        self.instrument_name = name
        self.facility_name = facility
        self._observers = []
        self._output_directory = os.path.expanduser('~')
        if HAS_MANTID:
            config = ConfigService.Instance()
            try:
                head, tail = os.path.split(config.getUserFilename())
                if os.path.isdir(head):
                    self._output_directory = head
            except:
                Logger("scripter").debug("Could not get user filename")

    def clear(self):
        """
            Clear out the observer list
        """
        self._observers = []

    def attach(self, subject):
        """
            Append a new widget to be observed
            @param subject: BaseWidget object
        """
        observer = BaseReductionScripter.ReductionObserver(subject)
        self._observers.append(observer)
        return observer

    def update(self):
        """
            Tell all observers to update their state.
        """
        for item in self._observers:
            item.update()

    def push_state(self):
        """
            Tell the observers to push their state to the their observed widget
        """
        for item in self._observers:
            item.push()

    def verify_instrument(self, file_name, id_element=None):
        """
            Verify that the current scripter object is of the right
            class for a given data file
            @param file_name: name of the file to check
            @param id_element: element name used to distinguish between formats
        """
        f = open(file_name, 'r')
        xml_str = f.read()
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Reduction")
        if len(element_list)>0:
            instrument_dom = element_list[0]
            found_name = BaseScriptElement.getStringElement(instrument_dom,
                                                            "instrument_name",
                                                            default=self.instrument_name).strip()
            # If we have an ID element and it can't be found, return None
            if id_element is not None:
                id_list = dom.getElementsByTagName(id_element)
                if len(id_list)==0:
                    return None
            return found_name
        else:
            raise RuntimeError, "The format of the provided file is not recognized"

    def check_xml_compatibility(self, file_name, id_element=None):
        """
            Check that an xml file is compatible with the current instrument
            @param file_name: path of the file to check
            @param id_element: if specified, we will check that an element of that name exists
        """
        try:
            instr = self.verify_instrument(file_name, id_element=id_element)
            return instr==self.instrument_name
        except:
            # Could not load file or identify it's instrument
            pass
        return False

    def to_xml(self, file_name=None):
        """
            Write all reduction parameters to XML
            @param file_name: name of the file to write the parameters to
        """
        xml_str = "<Reduction>\n"
        xml_str += "  <instrument_name>%s</instrument_name>\n" % self.instrument_name
        xml_str += "  <timestamp>%s</timestamp>\n" % time.ctime()
        xml_str += "  <python_version>%s</python_version>\n" % sys.version
        xml_str += "  <platform>%s</platform>\n" % platform.system()
        xml_str += "  <architecture>%s</architecture>\n" % str(platform.architecture())
        if HAS_MANTID:
            xml_str += "  <mantid_version>%s</mantid_version>\n" % version_str()

        for item in self._observers:
            if item.state() is not None:
                xml_str += item.state().to_xml()

        xml_str += "</Reduction>\n"

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(xml_str)
            f.close()

        return xml_str

    def _write_to_file(self, file_name, content):
        """
            Write content to a file
            @param file_name: file path
            @param content: content to be written
        """
        if file_name is not None:
            f = open(file_name, 'w')
            f.write(content)
            f.close()

    def from_xml(self, file_name, use_setup_info=False):
        """
            Read in reduction parameters from XML
            @param file_name: name of the XML file to read
        """
        f = open(file_name, 'r')
        xml_str = f.read()
        for item in self._observers:
            if item.state() is not None:
                if use_setup_info is False:
                    item.state().from_xml(xml_str)
                elif hasattr(item.state(), 'from_setup_info'):
                    item.state().from_setup_info(xml_str)

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        script = "# %s reduction script\n" % self.instrument_name
        script += "# Script automatically generated on %s\n\n" % time.ctime(time.time())

        for item in self._observers:
            if item.state() is not None:
                script += str(item.state())

        if file_name is not None:
            f = open(file_name, 'w')
            f.write(script)
            f.close()

        return script

    def to_batch(self):
        """
             @param script_dir: directory where to write the job scripts
        """
        return [self.to_script()]

    def apply(self):
        """
            Apply the reduction process to a Mantid SANSReducer
        """
        if HAS_MANTID:
            script = self.to_script(None)
            try:
                self.execute_script(script)
                # Update scripter
                for item in self._observers:
                    if item.state() is not None:
                        item.state().update()
            except:
                # Update scripter [Duplicated code because we can't use 'finally' on python 2.4]
                for item in self._observers:
                    if item.state() is not None:
                        # Things might be broken, so update what we can
                        try:
                            item.state().update()
                        except:
                            pass
                raise RuntimeError, sys.exc_value
        else:
            raise RuntimeError, "Reduction could not be executed: Mantid could not be imported"

    def apply_live(self):
        """
            Construct and execute a call to StartLiveData for the current instrument
        """
        script = self.to_live_script()
        self.execute_script(script)

    def cluster_submit(self, output_dir, user, pwd, resource=None,
                       nodes=4, cores_per_node=4, job_name=None):
        """
            Submit the reduction job to a cluster
            @param output_dir: directory where the output data will be written
            @param user: name of the user on the cluster
            @param pwd: password of the user on the cluster
        """
        Logger("scripter").notice("Preparing remote reduction job submission")

        if HAS_MANTID:
            # Generate reduction script and write it to file
            scripts = self.to_batch()
            for i in range(len(scripts)):
                script = scripts[i]
                script_name = "job_submission_%s.py" % i

                lower_case_instr = self.instrument_name.lower()
                job_name_lower = job_name.lower()
                _job_name = job_name
                if job_name is None or len(job_name)==0:
                    _job_name = lower_case_instr
                elif job_name_lower.find(lower_case_instr)>=0:
                    _job_name = job_name.strip()
                else:
                    _job_name = "%s_%s" % (lower_case_instr, job_name.strip())

                # Make sure we have unique job names
                if len(scripts)>1:
                    _job_name += "_%s" % i
                # Submit the job
                submit_cmd =  "Authenticate(ComputeResource='%s', " % resource
                submit_cmd += "UserName='%s', Password='%s')\n" % (user, pwd)

                submit_cmd += "id=StartRemoteTransaction(ComputeResource='%s')\n" % resource

                submit_cmd += "SubmitRemoteJob(ComputeResource='%s', " % resource
                submit_cmd += "TaskName='%s'," % _job_name
                submit_cmd += "NumNodes=%s, CoresPerNode=%s, " % (nodes, cores_per_node)
                submit_cmd += "TransactionID=id, "
                submit_cmd += "PythonScript=\"\"\"%s\"\"\", " % script
                submit_cmd += "ScriptName='%s')" % script_name
                mantidplot.runPythonScript(submit_cmd, True)
        else:
            Logger("scripter").error("Mantid is unavailable to submit a reduction job")

    def execute_script(self, script):
        """
            Executes the given script code.

            If MantidPlot is available it calls back to MantidPlot
            to ensure the code is executed asynchronously, if not
            then a simple exec call is used

            @param script :: A chunk of code to execute
        """
        if HAS_MANTIDPLOT:
            mantidplot.runPythonScript(script, True)
        else:
            exec script


    def reset(self):
        """
            Reset reduction state to default
        """
        for item in self._observers:
            item.reset()


