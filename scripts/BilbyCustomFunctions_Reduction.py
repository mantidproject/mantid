# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +from mantid.simpleapi import *

import csv
import math
from itertools import product
import sys
from mantid.simpleapi import MoveInstrumentComponent, CropWorkspace

attenuation_correction_pre_2016 = {1.0: 0.007655, 3.0: 1.0, 5.0: 0.005886}
attenuation_correction_post_2016 = {1.0: 1.0, 2.0: 0.00955, 3.0: 0.005886, 4.0: 0.00290, 5.0: 0.00062}


#######################################################################################
# REDUCTION ###########################################################################
#######################################################################################
def string_boolean(line):
    """ Convert string to boolean; needed to read "true" and "false" from the csv Data reduction settings table """

    if line == 'false':
        bool_string = False
    elif line == 'true':
        bool_string = True
    else:
        print("Check value of {}".format(line))
        print("It must be either True or False")
        sys.exit()
    return bool_string


#######################################################################################

def read_convert_to_float(array_strings):
    """ Needed to convert binning parameters from the csv file into the float numbers """

    array = [x.strip() for x in array_strings.split(',')]
    array = [float(x) for x in array]
    if (len(array) != 3):
        print("Check input parameters; binning parameters shall be given in a format left_value, step, right_value.")
        sys.exit()
    return array


###########################################################################################

def files_list_reduce(filename):
    """ Creat array of input reduction settings """
    parameters = []
    with open(filename) as csv_file:
        reader = csv.DictReader(csv_file)
        for row in reader:
            if row['index'] == '':
                continue
            if row['index'] == 'END':
                break
            parameters.append(row)
    return parameters


#######################################################################################
# ===============================================================
# Function to extract list of lines in the csv file to be processed; input format is a combination of digits, '-' ,',' or empty space
# returns list of numbers to be processed

def evaluate_files_list(numbers):
    """ Needed for files_to_reduce, see below """

    expanded = []
    for number in numbers.split(","):
        if "-" in number:
            start, end = number.split("-")
            nrs = range(int(start), int(end) + 1)
            expanded.extend(nrs)
        else:
            expanded.append(int(number))
    return expanded

    #######################################################################################


def files_to_reduce(parameters, evaluate_files):
    """ Create list of the files to reduce """

    files_to_reduce = []

    if len(evaluate_files) == 0:
        files_to_reduce.extend(parameters)
    else:
        evaluate_files_l = evaluate_files_list(evaluate_files)  # call funciton for retrieve the IDs list
        for parameter in parameters:
            if int(parameter['index']) in evaluate_files_l:
                files_to_reduce.append(parameter)

    return files_to_reduce


#######################################################################################

def strip_NaNs(output_workspace, base_output_name):
    """  Strip NaNs from the 1D OutputWorkspace """  # add isinf

    data = output_workspace.readY(0)
    start_index = next((index for index in range(len(data)) if not math.isnan(data[index])), None)
    end_index = next((index for index in range(len(data) - 1, -1, -1) if not math.isnan(data[index])), None)

    q_values = output_workspace.readX(0)
    start_q = q_values[start_index]
    end_q = q_values[end_index]

    CropWorkspace(InputWorkspace=output_workspace, XMin=start_q, XMax=end_q, OutputWorkspace=base_output_name)

    return base_output_name


#######################################################################################
# GENERAL #############################################################################
#######################################################################################

def get_pixel_size():  # reads current IDF and get pixelsize from there
    """ To get pixel size for Bilby detectors from the Bilby_Definition.xml file """

    from mantid.api import ExperimentInfo
    import xml.etree.cElementTree as ET

    currentIDF = ExperimentInfo.getInstrumentFilename("Bilby")
    # print currentIDF
    tree = ET.parse(currentIDF)
    for node in tree.iter():
        if node.tag == "{http://www.mantidproject.org/IDF/1.0}height":
            name = node.attrib.get('val')
            break
    pixelsize = float(name)

    return pixelsize


#######################################################################################

def read_csv(filename):
    """ Read cvs... """

    parameters = []
    with open(filename) as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            parameters.append(row)
    return parameters


#######################################################################################

def attenuation_correction(att_pos, data_before_May_2016):
    """ Bilby has four attenuators; before May 2016 there were only two.
     Value of the attenuators are hard coded here and being used for the I(Q) scaling in Q1D """

    if (data_before_May_2016):
        print("You stated data have been collected before May, 2016, i.e. using old attenuators. Please double check.")
        if (att_pos == 2.0 or att_pos == 4.0):
            print(
                "Wrong attenuators value; Either data have been collected after May, 2016, or something is wrong with hdf file")
            sys.exit()
        scale = attenuation_correction_pre_2016[att_pos]
    else:
        scale = attenuation_correction_post_2016[att_pos]
    return scale


#######################################################################################
# FOR WAVELENGTH SLICES ###############################################################
#######################################################################################
# To create a set of wavelength slices, if requested by reduction settings

def wavelengh_slices(wavelength_intervals, binning_wavelength_ini, wav_delta):
    """ This function defined number of wavelenth slices and creates array of the binning parameters for each slice """

    binning_wavelength = []
    if not wavelength_intervals:
        binning_wavelength.append(binning_wavelength_ini)
        n = 1  # in this case, number of wavelength range intervals always will be 1
    else:  # reducing data on a several intervals of wavelengths
        wav1 = float(binning_wavelength_ini[0])
        wv_ini_step = float(binning_wavelength_ini[1])
        wav2 = float(binning_wavelength_ini[2])

        # check if chosen wavelenth interval is feasible
        if (wav1 + wav_delta) > wav2:
            raise ValueError("wav_delta is too large for the upper range of wavelength")
        if math.fmod((wav2 - wav1), wav_delta) == 0.0:  # if reminder is 0
            n = (wav2 - wav1) / wav_delta
        else:  # if reminder is greater than 0, to trancate the maximum wavelength in the range
            n = math.floor((wav2 - wav1) / wav_delta)
            max_wave_length = wav1 + n * wav_delta
            print('WARNING: because of your set-up, maximum wavelength to consider for'
                  ' partial reduction is only %4.2f' % max_wave_length)

        # number of wavelength range intervals
        n = int(n)
        binning_wavelength_interm = []
        binning_wavelength_interm_1 = wv_ini_step  # binning step is always the same
        for i in range(n):
            binning_wavelength_interm_0 = wav1 + wav_delta * i  # left range
            binning_wavelength_interm_2 = binning_wavelength_interm_0 + wav_delta  # right range
            binning_wavelength_interm = [binning_wavelength_interm_0, binning_wavelength_interm_1,
                                         binning_wavelength_interm_2]
            binning_wavelength.append(binning_wavelength_interm)
        binning_wavelength.append(binning_wavelength_ini)  # reduce data on the full range
        n = n + 1  # to include full range

    return binning_wavelength, n


###############################################################


#######################################################################################
# FOR TUBE ADJUSTMENT #################################################################
#######################################################################################

def correction_tubes_shift(ws_to_correct, path_to_shifts_file):
    """ This function moves each tube and then rear panels as a whole as per numbers recorded in the path_to_shifts_file csv file.
          The values in the file are obtained from fitting of a few data sets collected using different masks.
          It is a very good idea do not change the file. """

    shifts = []
    # shall be precisely sevel lines; shifts for rear left, rear right, left, right, top, bottom curtains
    #  [calculated from 296_Cd_lines_setup1 file] + value for symmetrical shift for entire rear panels
    shifts = read_csv(
        path_to_shifts_file)
    pixelsize = get_pixel_size()

    correct_element_one_stripe("BackDetectorLeft", pixelsize, shifts[0], ws_to_correct)
    correct_element_one_stripe("BackDetectorRight", pixelsize, shifts[1], ws_to_correct)
    correct_element_one_stripe("CurtainLeft", pixelsize, shifts[2], ws_to_correct)
    correct_element_one_stripe("CurtainRight", pixelsize, shifts[3], ws_to_correct)
    correct_element_one_stripe("CurtainTop", pixelsize, shifts[4], ws_to_correct)
    correct_element_one_stripe("CurtainBottom", pixelsize, shifts[5], ws_to_correct)
    move_rear_panels(shifts[6][0], pixelsize, ws_to_correct)

    correction_based_on_experiment(ws_to_correct)

    return


#######################################################################################

def correct_element_one_stripe(panel, pixelsize, shift,
                               ws):  # sutable for one Cd stripe correction and for the stripes on BorAl mask on left curtain
    """ Technical for correction_tubes_shift """

    eightpack = ['eight_pack1', 'eight_pack2', 'eight_pack3', 'eight_pack4', 'eight_pack5']
    tube = ['tube1', 'tube2', 'tube3', 'tube4', 'tube5', 'tube6', 'tube7', 'tube8']

    i = 0
    for ei_pack, t_tube in product(eightpack, tube):
        if (panel == "BackDetectorLeft" or panel == "CurtainLeft"):
            direction = 1.0
            MoveInstrumentComponent(ws, panel + '/' + ei_pack + '/' + t_tube, X=0,
                                    Y=-float(shift[i]) * pixelsize * direction, Z=0)
        if (panel == "BackDetectorRight" or panel == "CurtainRight"):
            direction = -1.0
            MoveInstrumentComponent(ws, panel + '/' + ei_pack + '/' + t_tube, X=0,
                                    Y=-float(shift[i]) * pixelsize * direction, Z=0)
        if (panel == "CurtainBottom"):
            direction = 1.0
            MoveInstrumentComponent(ws, panel + '/' + ei_pack + '/' + t_tube,
                                    X=-float(shift[i]) * pixelsize * direction, Y=0, Z=0)
        if (panel == "CurtainTop"):
            direction = -1.0
            MoveInstrumentComponent(ws, panel + '/' + ei_pack + '/' + t_tube,
                                    X=-float(shift[i]) * pixelsize * direction, Y=0, Z=0)
        i = i + 1
    return ws


#######################################################################################

def move_rear_panels(shift, pixelsize,
                     ws):  # moves only rear left and rear right, each on shift; +1 to the right panel to make them symmetrical
    """ Technical for correction_tubes_shift """

    panel = "BackDetectorLeft"
    direction = 1.0
    MoveInstrumentComponent(ws, panel, X=0, Y=-float(shift) * pixelsize * direction, Z=0)

    panel = "BackDetectorRight"
    direction = -1.0
    MoveInstrumentComponent(ws, panel, X=0, Y=-float(shift) * pixelsize * direction, Z=0)

    return ws


#######################################################################################

def correction_based_on_experiment(ws_to_correct):
    """ The function to move curtains, based on fits/analysis of a massive set of AgBeh and liquid crystals data.
          Laser tracker has not picked up these imperfections.
         Added on October, 6th, 2016 """

    MoveInstrumentComponent(ws_to_correct, 'CurtainLeft', X=-5.3 / 1000, Y=0, Z=13.0 / 1000)
    MoveInstrumentComponent(ws_to_correct, 'CurtainRight', X=5.5 / 1000, Y=0, Z=17.0 / 1000)
    MoveInstrumentComponent(ws_to_correct, 'CurtainTop', X=0, Y=-4.0 / 1000, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'CurtainBottom', X=0, Y=6.0 / 1000, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'BackDetectorRight', X=0, Y=-2.0 / 1000, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'BackDetectorLeft', X=0, Y=-2.0 / 1000, Z=0)

    return


#######################################################################################

def det_shift_before_2016(ws_to_correct):
    """ Final detectors' alignement has been done using laser tracker in January, 2016.
     To correct data collected before that, some extra shift hardcoded here, shall be applied """

    shift_curtainl = 0.74 / 1000
    shift_curtainr = 6.92 / 1000
    shift_curtainu = -7.50 / 1000
    shift_curtaind = -1.59 / 1000

    MoveInstrumentComponent(ws_to_correct, 'CurtainLeft', X=shift_curtainl, Y=0, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'CurtainRight', X=shift_curtainr, Y=0, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'CurtainTop', X=0, Y=shift_curtainu, Z=0)
    MoveInstrumentComponent(ws_to_correct, 'CurtainBottom', X=0, Y=shift_curtaind, Z=0)

    correction_based_on_experiment(ws_to_correct)

#######################################################################################
