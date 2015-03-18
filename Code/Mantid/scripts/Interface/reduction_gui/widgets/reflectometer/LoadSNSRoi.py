#pylint: disable=invalid-name
import sys

"""
    This algorithm reads the SNS ROI files (used in M. Reuter data reduction).
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

    The range of pixel is then retrieved [5,6]

    Code contributed by Jean Bilheux (SNS)
"""

class LoadSNSRoi:

    x_list = []
    y_list = []
    mode = ""  #'narrow/broad" or 'discrete'
    pixel_range = []

    def __init__(self, filename=None):

        self.x_list = []
        self.y_list = []
        self.mode = ""
        self.pixel_range = []

        if filename:
            fh = open(filename)
            for line in fh:
                if line.startswith("#"):
                    continue
                self.retrieve_x_y_list(line)
            fh.close()
            self.x_list = list(set(self.x_list))
            self.y_list = list(set(self.y_list))
            self.calculatePixelRange()

    def retrieve_x_y_list(self, line):
        args = line.split('_')
        self.x_list.append(int(args[1]))
        self.y_list.append(int(args[2]))

    def getXlist(self):
        return self.x_list

    def getYlist(self):
        return self.y_list

    def getMode(self):
        return self.mode

    def getPixelRange(self):
        return self.pixel_range

    def calculatePixelRange(self):
        nbr_x = len(self.x_list)
        if (nbr_x % 304) == 0:
            _list = self.y_list #REF_L
        else:
            _list = self.x_list #REF_M

        sz = len(_list)
        Pixel_max = _list[-1]
        Pixel_min = _list[0]

        delta_pixel = Pixel_max - Pixel_min + 1

        if delta_pixel == sz:
            #narrow or broad
            self.mode = 'narrow/broad'
            self.pixel_range = [_list[0], _list[-1]]
        else:
            #discrete mode
            self.mode = 'discrete'
            self.retrieveDiscretePixelRange(_list)

    def retrieveDiscretePixelRange(self, _list):

        start_pixel = _list[0]
        left_pixel = _list[0]
        for right_pixel in _list[1:]:
            delta_pixel = right_pixel - left_pixel
            if delta_pixel != 1:
                new_range = [start_pixel, left_pixel]
                self.pixel_range.append(new_range)
                start_pixel = right_pixel
            left_pixel = right_pixel
        self.pixel_range.append([start_pixel, right_pixel])

    def retrieveFormatedDiscretePixelRange(self):
        _list = self.pixel_range
        _formated_string = ""
        for i in _list:
            _formated_string += " " + str(i)
        return _formated_string



