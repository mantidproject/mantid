import xml.dom.minidom
from scripter import BaseScriptElement
 
class ExampleState(BaseScriptElement):
    text = "My tailor is rich"
    alternate_text = "My garden is beautiful"
    
    def to_script(self):
        """
            Generate reduction script
        """
        script = "print \"%s\"" % self.text
        return script
        
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml = "<example_content>%s</example_content>\n" % self.text
        xml += "<alternate_content>%s</alternate_content>\n" % self.alternate_text
        return xml 

    def from_xml(self, xml_str):
        """
            Read in data from XML
            
            Helper class methods can be found in BaseScriptElement:

            @param xml_str: text to read the data from
        """ 
        self.reset()   
        dom = xml.dom.minidom.parseString(xml_str)
        self.text = BaseScriptElement.getStringElement(dom, "example_content")
        self.alternate_text = BaseScriptElement.getStringElement(dom, "alternate_content")
       
    def reset(self):
        """
            Reset state
        """
        self.text = ExampleState.text
        self.alternate_text = ExampleState.alternate_text
