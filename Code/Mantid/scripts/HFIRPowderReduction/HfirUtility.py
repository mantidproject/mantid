################################################################################
#
# Utility methods for HFIR powder reduction
#
################################################################################


def makeHB2ADetEfficiencyFileName(expno, m1, colltrans):
    """ Fabricate the detector's efficiency file name for HB2A
    Example: HB2A_exp0400__Ge_113_IN_vcorr.txt

   * Ge 113: :math:`\lambda = 2.41 \AA`, m1 = 9.45
   * Ge 115: :math:`\lambda = 1.54 \AA`, m1 = 0
   * Ge 117  :math:`\lambda = 1.12 \AA`, No used

    Arguments: 
     - expno :: experiment number
     - m1 :: Ge setup for neutron wavelength (m1)
     - colltrans :: for In/Out

    Return :: 3-tuple (filename, filename with URL, wavelength)
    """
    # Determine wavelength setup
    wavelengthsetup = None
    m1 = float(m1)
    if abs(m1 - 9.45) < 0.1:
        # m1 = 9.45
        wavelength = 2.41
        wavelengthsetup = 'Ge_113'
    elif abs(m1) < 0.1:
        # m1 = 0.
        wavelength = 1.54
        wavelengthsetup = 'Ge_115'
    else:
        # not defined
        raise NotImplementedError("'m1' value %f is not defined for wavelength setup." % (m1))

    # Determine In/Out, i.e., collimator trans 
    if colltrans is not None:
        # colltrans is in sample log
        colltrans = int(colltrans)
        if abs(colltrans) == 80:
            collimator = 'OUT'
        elif colltrans == 0:
            collimator = 'IN'
        else:
            raise NotImplementedError("'colltrans' equal to %d is not defined for IN/OUT." % (colltrans))

        # Make the detector efficiency file name
        expno = int(expno)
        defefffilename = 'HB2A_exp%04d__%s_%s_vcorr.txt' % (expno, wavelengthsetup, collimator)
        url = 'http://neutron.ornl.gov/user_data/hb2a/exp%d/Datafiles/%s' % (expno, defefffilename)
    else:
        # old style, no colltrans
        defefffilename = None
        url = None
    # ENDIFELSE

    return (defefffilename, url, wavelength)


def makeExcludedDetectorFileName(expno):
    """ Make the excluded detectors' file name

    Return :: 2-tuple (file name, URL)
    """
    expno = int(expno)
    excludeddetfilename = 'HB2A_exp%04d__exclude_detectors.txt' % (expno)
    url = 'http://neutron.ornl.gov/user_data/hb2a/exp%d/Datafiles/%s' % (expno, excludeddetfilename) 

    return (excludeddetfilename, url)


def makeDetGapFileName(expno):
    """ Make the detectors' gap file name

    Return :: 2-tuple (file name, URL)
    """
    expno = int(expno)
    detgapfilename = 'HB2A_exp04d__gaps.txt' % (expno)
    url = 'http://neutron.ornl.gov/user_data/hb2a/exp%d/Datafiles/%s' % (expno, detgapfilename)

    return (detgapfilename, url)
