import sys
from reduction_gui.settings.application_settings import GeneralSettings

"""
    This algorithm will create the SNS ROI files (used in M. Reuter data reduction).
    Format of file:
        bank1_5_1
        bank1_5_2
        bank1_5_3
        ....
        bank1_5_256
        bank1_6_1
        bank1_6_2
        bank1_6_3
        ....
        bank1_6_256
        
    using a range of pixels such as [5,6], OR [[5,8],[10,15]]
    
    Code contributed by Jean Bilheux (SNS)
"""

class SaveSNSRoi:
    
    x_list = []
    y_list = []
    mode = ""  #'narrow/broad" or 'discrete'
    pixel_max = 0
    index_pixel_of_interst = 0
    other_pixel_range = []
    file_array = []
    
    def __init__(self, filename=None, pixel_range=None, mode=None):
        
        if GeneralSettings.instrument_name == 'REF_L':
            self.pixel_max = 304
            self.other_pixel_range = range(self.pixel_max)
            self.index_pixel_of_interest = 1
        else:
            self.pixel_max = 256
            self.other_pixel_range = range(self.pixel_max)
            self.index_pixel_of_interest = 0
        
        if mode == 'discrete': #discrete
            nbr_discrete = len(pixel_range)
            for _range in pixel_range:
                _start_pixel = _range[0]
                _end_pixel = _range[1]
                _discrete_range = range(_start_pixel,_end_pixel+1)
                for i in _discrete_range:
                    for j in self.other_pixel_range:
                        _line = 'bank1_'
                        if self.index_pixel_of_interest == 0:
                            _line += str(i) + '_' + str(j) + '\n'
                        else:
                            _line += str(j) + '_' + str(i) + '\n'
                        self.file_array.append(_line)

        else: #narrow/broad
            _start_pixel = pixel_range[0]
            _end_pixel = pixel_range[1]
            _discrete_range = range(_start_pixel,_end_pixel+1)
            for i in _discrete_range:
                for j in self.other_pixel_range:
                    _line = 'bank1_'
                    if self.index_pixel_of_interest == 0:
                        _line += str(i) + '_' + str(j) + '\n'
                    else:
                        _line += str(j) + '_' + str(i) + '\n'
                    self.file_array.append(_line)
        
        #Write array into file
        if filename:
            fh = open(filename, 'w')
            for line in self.file_array:
                fh.write(line)
            fh.close()
 
        
        
            