.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
Provide input files for the program LAUENORM which is used to perform a wavelength normalisation for
Laue data using symmetry equivalent reflections measured at different wavelengths.

Input_Files

Unit 21     LAUE001      Input Laue data file.  This is normally a card image file with one  record  per  reflection (unmerged, unsorted data) containing the items: h k l lambda theta intensity and sig(intensity) in format (3I5,2F10.5,2I10).

Unit 22     LAUE002

Unit 23     LAUE003

Continuing

From: http://www.ccp4.ac.uk/cvs/viewvc.cgi/laue/doc/lauenorm.ptx?diff_format=s&revision=1.1.1.1&view=markup

Usage
-----
**Example - a simple example of running SaveLauenorm.**

.. testcode:: ExSaveLauenormSimple

    import os

    prefix = os.path.expanduser("~/MyPeaks")
    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    SaveLauenorm(InputWorkspace=peaks, Filename=prefix)

    firstfile = prefix + "001"
    print "File was saved:", os.path.isfile(firstfile)

Output:

.. testoutput:: ExSaveLauenormSimple

    File was saved: True

.. testcleanup:: ExSaveLauenormSimple

    import os
    def removeFiles(files):
      for ws in files:
        try:
          os.remove(os.path.join(os.path.expanduser("~"), ws))
        except IOError:
          pass

    removeFiles(["MyPeaks001","MyPeaks002","MyPeaks003","MyPeaks004",
                 "MyPeaks005","MyPeaks006","MyPeaks007","MyPeaks008","MyPeaks009"])

**Example - an example of running SaveLauenorm with sorting and filtering options.**

.. testcode:: ExSaveLauenormOptions

    import os

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    print "Number of peaks in table %d" % peaks.rowCount()

    prefix = os.path.expanduser("~/MyPeaks")
    SaveLauenorm(InputWorkspace=peaks, Filename=prefix, MinWavelength=0.5, MaxWavelength=2,MinDSpacing=0.2, SortFilesBy='Bank')

    finalfile = prefix + "009"
    ifile = open(finalfile, 'r')
    lines = ifile.readlines()
    ifile.close()
    print "Number of peaks in table %d" % len(lines)

Output:

.. testoutput:: ExSaveLauenormOptions

    Number of peaks in table 434
    Number of peaks in table 23

.. testcleanup:: ExSaveLauenormOptions

    import os
    def removeFiles(files):
      for ws in files:
        try:
          os.remove(os.path.join(os.path.expanduser("~"), ws))
        except IOError:
          pass

    removeFiles(["MyPeaks001","MyPeaks002","MyPeaks003","MyPeaks004","MyPeaks005","MyPeaks006","MyPeaks007","MyPeaks008","MyPeaks009"])



.. categories::
