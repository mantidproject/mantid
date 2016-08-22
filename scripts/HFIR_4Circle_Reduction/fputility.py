#pylint: disable=R0914
# Utility methods for Fullprof


def write_scd_fullprof_kvector(user_header, wave_length, k_vector_dict, peak_dict_list, fp_file_name):
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
    file_format = '(4i4,2f8.2,i4,3f8.2)'
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
    for i_peak, peak_dict in enumerate(peak_dict_list):
        # check
        assert isinstance(peak_dict, dict), '%d-th peak must be a dictionary but not %s.' % (i_peak,
                                                                                             str(type(peak_dict)))
        for key in ['hkl', 'kindex', 'intensity', 'sigma']:
            assert key in peak_dict, '%d-th peak dictionary does not have required key %s.' % (i_peak, key)

        # miller index
        m_h, m_k, m_l = peak_dict['hkl']
        part1 = '%4d%4d%4d' % (round(m_h), round(m_k), round(m_l))

        # k index
        if num_k_vectors > 0:
            k_index = peak_dict['kindex']
            if k_index < 1 and num_k_vectors > 0:
                raise RuntimeError('Exporting file with k-vector shift. But a peak without information on shift '
                                   'k-vector is found.')
            part2 = '%4d' % k_index
        else:
            part2 = ''
        # END-IF-ELSE

        # peak intensity and sigma
        part3 = '%8.2f%8.2f   1' % (peak_dict['intensity'], peak_dict['sigma'])

        peak_line = part1 + part2 + part3

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
