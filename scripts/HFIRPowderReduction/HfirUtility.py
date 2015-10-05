#pylint: disable=invalid-name
################################################################################
#
# Utility methods for HFIR powder reduction
#
################################################################################


def makeHB2ADetEfficiencyFileName(expno, m1, colltrans):
    """ Fabricate the detector's efficiency file name for HB2A
    Example: HB2A_exp0400__Ge_113_IN_vcorr.txt

   * Ge 113: lambda = 2.41 A, m1 = 9.45
   * Ge 115: lambda = 1.54 A, m1 = 0
   * Ge 117  lambda = 1.12 A, No used

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


def parseDetEffCorrFile(vancorrfname):
    """ Parse HB2A vanadium correction (detector efficiency correction) file
    Return :: dictionary : key = det id, value = factor
    """
    try:
        cfile = open(vancorrfname, 'r')
        lines = cfile.readlines()
        cfile.close()
    except IOError:
        return (False, 'Unable to read vanadium correction file %s.'%(vancorrfname))

    corrdict = {}
    detid = 1
    for line in lines:
        line = line.strip()
        if len(line) == 0 or line[0] == '#':
            continue

        terms = line.split()
        factor = float(terms[0])
        corrdict[detid] = factor

        detid += 1
    # ENDFOR

    return (corrdict, '')


def parseDetExclusionFile(detexludefilename):
    """ Parse excluded detectors file
    Detector ID from standard HB2A detector exclusion file start from 0,
    while in the other circumstance, it starts from 1.
    Therefore Det ID output from here must be plus by 1.
    Return :: 2-tuple
      Success: Excluded detector IDs list, empty string
      Fail: None, error message
    """
    try:
        defile = open(detexludefilename)
        lines = defile.readlines()
        defile.close()
    except IOError:
        return (None, 'Unable to read excluded detector file %s.'%(detexludefilename))

    detexcludelist = []
    for line in lines:
        line = line.strip()
        if len(line) == 0 or line[0] == '#':
            continue

        terms = line.split()
        for term in terms:
            try:
                detid = int(term) + 1
                detexcludelist.append(detid)
            except ValueError:
                break
        # ENDFOR
    # ENDFOR

    return (detexcludelist, '')
