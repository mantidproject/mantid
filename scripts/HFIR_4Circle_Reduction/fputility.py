#pylint: disable=R0914
# Utility methods for Fullprof


def write_scd_fullprof_kvector(user_header, wave_length, k_vector_dict, peak_dict_list, fp_file_name,
                               with_absorption):
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
    :return:
    """
    # check
    assert isinstance(user_header, str), 'User header must be a string.'
    assert isinstance(wave_length, float), 'Neutron wave length must be a float.'
    assert isinstance(k_vector_dict, dict), 'K-vector list must be a dictionary.'
    assert isinstance(peak_dict_list, list), 'Peak-dictionary list must be a list.'

    # set up all lines
    fp_buffer = ''

    # user defined header
    header = '%s' % user_header.strip()
    # fixed file format line
    if with_absorption:
        file_format = '(4i4,2f8.2,i4,3f8.5)'
    else:
        file_format = '(4i4,2f8.2,i4)'
    # wave length
    lambda_line = '%.4f  0  0' % wave_length

    fp_buffer += header + '\n' + file_format + '\n' + lambda_line + '\n'

    # tricky one.  number of K vectors
    num_k_vectors = len(k_vector_dict)
    if num_k_vectors > 0:
        # number of k vectors
        kline = '%d' % num_k_vectors

        for k_index in k_vector_dict.keys():
            k_vector = k_vector_dict[k_index]
            # write k_x, k_y, k_z
            kline += '\n%d %.3f    %.3f    %.3f' % (k_index, k_vector[0], k_vector[1], k_vector[2])

        fp_buffer += kline + '\n'
    # END-IF

    # peak intensities
    print '[DB...BAT] Number of peaks to output: ', len(peak_dict_list)

    for i_peak, peak_dict in enumerate(peak_dict_list):
        # check
        assert isinstance(peak_dict, dict), '%d-th peak must be a dictionary but not %s.' % (i_peak,
                                                                                             str(type(peak_dict)))
        for key in ['hkl', 'kindex', 'intensity', 'sigma']:
            assert key in peak_dict, '%d-th peak dictionary does not have required key %s.' % (i_peak, key)

        # check whether it is magnetic
        if num_k_vectors > 0 and peak_dict['kindex'] > 0:
            is_magnetic = True
        else:
            is_magnetic = False

        # miller index
        m_h, m_k, m_l = peak_dict['hkl']
        if is_magnetic:
            k_index = peak_dict['kindex']
            kx, ky, kz = k_vector_dict[k_index]
        else:
            kx = ky = kz = 0.0
            k_index = 0

        # remove the magnetic k-shift vector from HKL
        part1 = '%4d%4d%4d' % (nearest_int(m_h-kx), nearest_int(m_k-ky), nearest_int(m_l-kz))

        # k index
        if is_magnetic:
            part2 = '%4d' % k_index
        else:
            part2 = ''
        """
        if num_k_vectors > 0:
            k_index = peak_dict['kindex']
            if k_index > 0:
                part2 = '%4d' % k_index
            else:
                part2 = ''
        else:
            part2 = ''
        """
        # END-IF-ELSE

        # peak intensity and sigma
        part3 = '%8.2f%8.2f%4d' % (peak_dict['intensity'], peak_dict['sigma'], 1)

        peak_line = part1 + part2 + part3

        # absorption
        if 'up' in peak_dict:
            part4 = ''
            for i in range(3):
                part4 += '%8.5f%8.5f' % (peak_dict['up'][i], peak_dict['us'][i])
            peak_line += part4

        fp_buffer += peak_line + '\n'
    # END-FOR (peak_dict)

    # write to file
    try:
        ofile = open(fp_file_name, 'w')
        ofile.write(fp_buffer)
        ofile.close()
    except IOError as io_err:
        err_msg = 'Unable to write to Fullprof single crystal file at %s due to %s..' % (fp_file_name, str(io_err))
        raise RuntimeError(err_msg)

    return fp_buffer


def nearest_int(number):
    """
    """
    if number > 0:
        ni = int(number + 0.5)
    else:
        ni = int(number - 0.5)

    return ni
