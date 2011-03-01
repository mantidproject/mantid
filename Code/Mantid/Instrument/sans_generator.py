"""
"""
import time
import sys

DEFINITION_TEMPLATE = "sans_Definition_template.txt"
PARAMETERS_TEMPLATE = "sans_Parameters_template.txt"

class Generator(object):
    ## Pixel size in X [mm]
    pixel_size_x = 5.15/1000.0
    ## Pixel size in Y [mm]
    pixel_size_y = 5.15/1000.0
    
    ## Instrument name
    name = "GPSANS"
    
    ## Number of pixels
    n_pixels_x = 192
    n_pixels_y = 192
    
    def __init__(self, name="GPSANS", pixel_size_x=5.15, pixel_size_y=5.15):
        self.name = name
    
    
    def __call__(self):
        
        self._create_definition()
        self._create_parameters()
        
    def _create_definition(self):
        output = open("%s_Definition.xml" % self.name.upper(), 'w')
        input = open(DEFINITION_TEMPLATE, 'r')
        for l in input.readlines():
            l=l.replace("%instrument_name%", self.name.upper())
            l=l.replace("%last_modified%", time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
            l=l.replace("%pixel_positions%", self._get_positions())
            l=l.replace("%pixels_ids%", self._get_ids())
        
            half_x = self.pixel_size_x/2.0
            half_y = self.pixel_size_y/2.0
            
            l=l.replace("%pixel_left_front_bottom_x%", "%6.6f" % -half_x)
            l=l.replace("%pixel_left_front_bottom_y%", "%6.6f" % -half_y)
            l=l.replace("%pixel_left_front_top_x%", "%6.6f" % -half_x)
            l=l.replace("%pixel_left_front_top_y%", "%6.6f" % half_y)
            l=l.replace("%pixel_left_back_bottom_x%", "%6.6f" % -half_x)
            l=l.replace("%pixel_left_back_bottom_y%", "%6.6f" % -half_y)
            l=l.replace("%pixel_right_front_bottom_x%", "%6.6f" % half_x)
            l=l.replace("%pixel_right_front_bottom_y%", "%6.6f" % -half_y)
            output.write(l)

        input.close()
        output.close()
        
    def _get_positions(self):
        positions = ""
        for i_x in range(self.n_pixels_x):
            for i_y in range(self.n_pixels_y):
                x = self.pixel_size_x * (i_x-self.n_pixels_x/2.0+0.5)
                y = self.pixel_size_y * (i_y-self.n_pixels_y/2.0+0.5)
                positions += "      <location y=\"%6.6f\" x=\"%6.6f\" />\n" % (y, x)
        
        return positions
    
    def _get_ids(self):
        ids = ""
        for i in range(self.n_pixels_y):
            start = 1000000 + i
            end = 1000000 + (self.n_pixels_x-1)*1000 + i
            ids += "    <id start=\"%d\" step=\"1000\" end=\"%d\" />\n" % (start, end)
        return ids
    
    def _create_parameters(self):
        output = open("%s_Parameters.xml" % self.name.upper(), 'w')
        input = open(PARAMETERS_TEMPLATE, 'r')
        for l in input.readlines():
            l=l.replace("%instrument_name%", self.name.upper())
            l=l.replace("%last_modified%", time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()))
            l=l.replace("%n_x_pixels%", "%d" % self.n_pixels_x)
            l=l.replace("%n_y_pixels%", "%d" % self.n_pixels_y)
            l=l.replace("%pixel_size_x%", "%7.7f" % (self.pixel_size_x*1000.0))
            l=l.replace("%pixel_size_y%", "%7.7f" % (self.pixel_size_y*1000.0))
            output.write(l)

        input.close()
        output.close()
         
    
if __name__ == '__main__':
    g = Generator(*sys.argv[1:])()