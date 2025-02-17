# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=R0914,R0912,R0913
# Utility methods for Fullprof
import os
import math


def calculate_intensity_difference(reflection_dict1, reflection_dict2):
    """
    Calculate the difference of the intensities on same reflection between 2 sets of measurements
    :param reflection_dict1:
    :param reflection_dict2:
    :return:
    """
    # check validity
    assert isinstance(reflection_dict1, dict), "Input 1 must be a dictionary"
    assert isinstance(reflection_dict2, dict), "Input 2 must be a dictionary"

    # get a list of HKL
    hkl_list = sorted(reflection_dict1.keys())

    # output
    out_dict = dict()

    for hkl in hkl_list:
        # skip if the HKL does not exist in both sets
        if hkl not in reflection_dict2:
            continue

        intensity_1, var_1 = reflection_dict1[hkl]
        intensity_2, var_2 = reflection_dict2[hkl]

        diff_intensity = intensity_1 - intensity_2
        diff_var = math.sqrt(var_1**2 + var_2**2)

        out_dict[hkl] = (diff_intensity, diff_var)
    # END-FOR

    return out_dict


def load_scd_fullprof_intensity_file(file_name):
    """
    load a single crystal diffraction Fullprof intensity file
    :param file_name:
    :return: 2-tuple.  dictionary for reflection (key = hkl, value = (intensity, error)); string as error message
    """
    # check validity
    assert isinstance(file_name, str), "Fullprof SCD intensity file %s must be a string but not of type %s." % (
        str(file_name),
        type(file_name),
    )
    assert os.path.exists(file_name), "Fullprof SCD intensity file %s cannot be found." % file_name

    # open file
    scd_int_file = open(file_name, "r")
    raw_lines = scd_int_file.readlines()
    scd_int_file.close()

    # parse file
    wave_length = 0.0
    num_k_vector = 0
    k_index = 0
    error_buffer = ""
    reflection_dict = dict()  # key: 3-tuple as (h, k, l)
    for line_index, raw_line in enumerate(raw_lines):
        # clean the line
        line = raw_line.strip()
        if len(line) == 0:
            continue

        if line_index == 0:
            # line 1 as header
            pass
        elif line.startswith("("):
            # line 2 format line, skip
            continue
        elif line.endswith("0  0"):
            # line 3 as wave length line
            wave_length = float(line.split()[0])
        elif k_index < num_k_vector:
            # k-vector line: (num_k_vector) line right after k-indication line
            k_index += 1
        else:
            # split
            terms = line.split()
            if len(terms) == 1:
                # k-vector
                num_k_vector = int(terms[0])
                continue

            # line that cannot be parsed
            if len(terms) < 5:
                # some line may have problem. print out and warning
                error_buffer += "unable to parse line %-3d: %s\n" % (line_index, line)
                continue

            try:
                lattice_h = int(terms[0])
                lattice_k = int(terms[1])
                lattice_l = int(terms[2])
                intensity = float(terms[3])
                variation = float(terms[4])
            except ValueError:
                error_buffer += "unable to parse line %-3d: %s\n" % (line_index, line)
            else:
                reflection_dict[(lattice_h, lattice_k, lattice_l)] = (intensity, variation)

        # END-IF-ELSE
    # END-FOR

    return reflection_dict, wave_length, error_buffer


def convert_to_peak_dict_list(refection_dict):
    """
    Convert Reflection dictionary to peak dictionary for writing out to Fullprof format
    :param refection_dict:
    :return:
    """
    # check validity
    assert isinstance(refection_dict, dict)

    peak_dict_list = list()
    # loop around all HKL
    for hkl in sorted(refection_dict.keys()):
        intensity, sigma = refection_dict[hkl]
        peak_dict = {"hkl": hkl, "kindex": 0, "intensity": intensity, "sigma": sigma}

        peak_dict_list.append(peak_dict)

    return peak_dict_list


def write_scd_fullprof_kvector(user_header, wave_length, k_vector_dict, peak_dict_list, fp_file_name, with_absorption, high_precision):
    """
    Purpose: Export integrated peaks to single crystal diffraction Fullprof file
    Requirements:
    Guarantees: a Fullprof is written
    Note:
      1. peak parameter dictionary: keys are 'hkl', 'kindex', 'intensity', and 'sigma'
    :param user_header: user defined header (information)
    :param wave_length: wavelength
    :param k_vector_dict:
    :param peak_dict_list: a list of peak parameters stored in dictionaries.
    :param fp_file_name:
    :param with_absorption:
    :param high_precision:
    :return:
    """
    # check input validity
    assert isinstance(user_header, str), "User header must be a string."
    assert isinstance(wave_length, float), "Neutron wave length must be a float."
    assert isinstance(k_vector_dict, dict), "K-vector list must be a dictionary."
    assert isinstance(peak_dict_list, list), "Peak-dictionary list must be a list."
    assert isinstance(high_precision, bool), "blabla"

    # determine whether the output is for magnetic peaks or nuclear peaks
    # assuming that either all peaks are magnetic or all peaks are nuclear
    num_k_vectors = len(k_vector_dict)
    peak_0_dict = peak_dict_list[0]
    assert isinstance(peak_0_dict, dict) and "kindex" in peak_0_dict, "blabla"
    peak0_is_magnetic = peak_0_dict["kindex"] > 0 and num_k_vectors > 0

    # set up fullprof .ini file buffer
    fp_buffer = ""

    # user defined header
    header = "COMM %s" % user_header.strip()
    # fixed file format line: Only  magnetic case use 4i4,2f8,...
    if peak0_is_magnetic:
        first_3_terms_foramt = "4i4"
    else:
        first_3_terms_foramt = "3i4"

    if with_absorption:
        file_format = "(%s,2f8.2,i4,6f8.5)" % first_3_terms_foramt
    elif high_precision:
        # precisions: (3i4,2f18.5,i4)
        file_format = "({0},2f18.5,i4)".format(first_3_terms_foramt)
    else:
        # precisions: (3i4,2f8.2,i4)
        file_format = "(%s,2f8.2,i4)" % first_3_terms_foramt
    # END-IF

    # wave length
    lambda_line = "%.4f  0  0" % wave_length

    fp_buffer += header + "\n" + file_format + "\n" + lambda_line + "\n"

    # tricky one.  number of K vectors
    if num_k_vectors > 0:
        # number of k vectors
        kline = "%d" % num_k_vectors

        for k_index in k_vector_dict.keys():
            k_vector = k_vector_dict[k_index]
            # write k_x, k_y, k_z
            kline += "\n%d %.3f    %.3f    %.3f" % (k_index, k_vector[0], k_vector[1], k_vector[2])

        fp_buffer += kline + "\n"
    # END-IF

    # again in body,
    if high_precision:
        # '%18.5f%18.5f%4d'
        part3_format = "{0:18.5f}{1:18.5f}{2:4d}"
    else:
        part3_format = "%8.2f%8.2f%4d"

    # peak intensities
    for i_peak, peak_dict in enumerate(peak_dict_list):
        # check
        assert isinstance(peak_dict, dict), "%d-th peak must be a dictionary but not %s." % (i_peak, str(type(peak_dict)))
        for key in ["hkl", "kindex", "intensity", "sigma"]:
            assert key in peak_dict, "%d-th peak dictionary does not have required key %s." % (i_peak, key)

        # check whether it is magnetic
        if num_k_vectors > 0 and peak_dict["kindex"] > 0:
            is_magnetic = True
        else:
            is_magnetic = False

        # miller index
        m_h, m_k, m_l = peak_dict["hkl"]
        if is_magnetic:
            k_index = peak_dict["kindex"]
            k_x, k_y, k_z = k_vector_dict[k_index]
        else:
            k_x = k_y = k_z = 0.0
            k_index = 0

        # remove the magnetic k-shift vector from HKL
        part1 = "%4d%4d%4d" % (nearest_int(m_h - k_x), nearest_int(m_k - k_y), nearest_int(m_l - k_z))

        # k index
        if is_magnetic:
            part2 = "%4d" % k_index
        else:
            part2 = ""
        # END-IF-ELSE

        # peak intensity and sigma
        try:
            if high_precision:
                part3 = part3_format.format(peak_dict["intensity"], peak_dict["sigma"], 1)
            else:
                part3 = "%8.2f%8.2f%4d" % (peak_dict["intensity"], peak_dict["sigma"], 1)
        except TypeError as type_err:
            raise RuntimeError(
                "In writing Fullprof file, unable to convert intensity {0} and/or sigma {1} to floats. FYI: {2}".format(
                    peak_dict["intensity"], peak_dict["sigma"], type_err
                )
            )

        peak_line = part1 + part2 + part3

        # absorption
        if "up" in peak_dict:
            part4 = ""
            for i in range(3):
                part4 += "%8.5f%8.5f" % (peak_dict["up"][i], peak_dict["us"][i])
            peak_line += part4

        fp_buffer += peak_line + "\n"
    # END-FOR (peak_dict)

    # write to file
    try:
        ofile = open(fp_file_name, "w")
        ofile.write(fp_buffer)
        ofile.close()
    except IOError as io_err:
        err_msg = "Unable to write to Fullprof single crystal file at %s due to %s.." % (fp_file_name, str(io_err))
        raise RuntimeError(err_msg)

    return fp_buffer


def nearest_int(number):
    """ """
    if number > 0:
        answer = int(number + 0.5)
    else:
        answer = int(number - 0.5)

    return answer
