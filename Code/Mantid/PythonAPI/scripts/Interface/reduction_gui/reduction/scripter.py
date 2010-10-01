"""
    Reduction scripter used to take reduction parameters
    end produce a Mantid reduction script
"""

class BaseScriptElement(object):
    """
        Base class for each script element (panel on the UI).
        Contains only data and is UI implementation agnostic.
    """
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
    def getFloatElement(cls, dom, tag, default=None):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return float(value)
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
    def getBoolElement(cls, dom, tag, true_tag='true', default=False):
        value = BaseScriptElement.getContent(dom, tag)
        if value is not None:
            return value.lower()==true_tag.lower()
        else:
            return default


class BaseReductionScripter(object):
    """
        Organizes the set of reduction parameters that will be used to
        create a reduction script. Parameters are organized by groups that
        will each have their own UI representation.
    """
    
    def __init__(self, name=""):
        self.instrument_name = name

    def to_xml(self, file_name=None):
        """
            Write all reduction parameters to XML
            @param file_name: name of the file to write the parameters to 
        """
        xml_str = "<Reduction>\n"
        xml_str += "</Reduction>\n"
            
        self._write_to_file(file_name, xml_str)
            
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
        
    def from_xml(self, file_name):
        """
            Read in reduction parameters from XML
            @param file_name: name of the XML file to read
        """
        return NotImplemented

    def to_script(self, file_name=None):
        """
            Spits out the text of a reduction script with the current state.
            @param file_name: name of the file to write the script to
        """
        return ""
    
    def apply(self):
        """
            Apply the reduction process to a Mantid SANSReducer.
            Returns a log of the reduction process.
        """
        return ""
