"""
    Module that defines the implementation of script elements common
    to all SANS instruments
"""
import xml.dom.minidom
import copy
import os
from scripter import BaseScriptElement

# Check whether Mantid is available
try:
    from MantidFramework import *
    mtd.initialise(False)
    from reduction.instruments.sans.hfir_command_interface import *
    HAS_MANTID = True
except:
    HAS_MANTID = False  

class Mask(BaseScriptElement):
    
    class RectangleMask(object):
        def __init__(self, x_min=0, x_max=0, y_min=0, y_max=0):
            self.x_min = x_min
            self.x_max = x_max
            self.y_min = y_min
            self.y_max = y_max
        
    # Edges
    top = 0
    bottom = 0
    left = 0
    right = 0
    
    # Shapes
    shapes = []
    
    def to_script(self):
        """
            Generate reduction script
            @param execute: if true, the script will be executed
        """
        script = ""
        
        # Edges
        if (not self.top == self.bottom and not self.left == self.bottom):
            script += "Mask(nx_low=%d, nx_high=%d, ny_low=%d, ny_high=%d)\n" % (self.left, self.right, self.bottom, self.top)

        # Rectangles
        for item in self.shapes:
            script += "MaskRectangle(x_min=%g, x_max=%g, y_min=%g, y_max=%g)\n" % (item.x_min, item.x_max, item.y_min, item.y_max)

        return script
    
    def to_xml(self):
        """
            Create XML from the current data.
        """
        xml  = "<Mask>\n"
        xml += "  <mask_top>%g</mask_top>\n" % self.top
        xml += "  <mask_bottom>%g</mask_bottom>\n" % self.bottom
        xml += "  <mask_left>%g</mask_left>\n" % self.left
        xml += "  <mask_right>%g</mask_right>\n" % self.right
        
        xml += "  <Shapes>\n"
        
    
        for item in self.shapes:
            xml += "    <rect x_min='%g' x_max='%g' y_min='%g' y_max='%g' />\n" % (item.x_min, item.x_max, item.y_min, item.y_max)
        xml += "  </Shapes>\n"
        
        xml += "</Mask>"
        return xml
    
    def from_xml(self, xml_str):
        """
            Read in data from XML
            @param xml_str: text to read the data from
        """       
        dom = xml.dom.minidom.parseString(xml_str)
        element_list = dom.getElementsByTagName("Mask")
        if len(element_list)>0: 
            mask_dom = element_list[0]
            self.top = BaseScriptElement.getIntElement(mask_dom, "mask_top", default=Mask.top)
            self.bottom = BaseScriptElement.getIntElement(mask_dom, "mask_bottom", default=Mask.bottom)
            self.right = BaseScriptElement.getIntElement(mask_dom, "mask_right", default=Mask.right)
            self.left = BaseScriptElement.getIntElement(mask_dom, "mask_left", default=Mask.left)
            
            self.shapes = []
            shapes_dom_list = mask_dom.getElementsByTagName("Shapes")
            if len(shapes_dom_list)>0:
                shapes_dom = shapes_dom_list[0]
                for item in shapes_dom.getElementsByTagName("rect"):
                    x_min =  float(item.getAttribute("x_min"))
                    x_max =  float(item.getAttribute("x_max"))
                    y_min =  float(item.getAttribute("y_min"))
                    y_max =  float(item.getAttribute("y_max"))
                    self.shapes.append(Mask.RectangleMask(x_min, x_max, y_min, y_max))
                            

    def reset(self):
        """
            Reset state
        """
        self.top = Mask.top
        self.bottom = Mask.bottom
        self.left = Mask.left
        self.right = Mask.right
        
        self.shapes = Mask.shapes
        
    
