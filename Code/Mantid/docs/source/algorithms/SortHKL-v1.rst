.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Peaks are sorted first by H, then K, and then L. For equivalent HKL in
the point group, the intensity is averaged and all the equivalent HKLs
have the same average intensity. Outliers with zscore > 3 from each
group of equivalent HKLs are not included in the average.

Usage
-----

**Example - an example of running SortHKL with PointGroup option.**

.. testcode:: ExSortHKLOption

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    peak = peaks.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()
    
    pw,chi2 = SortHKL(InputWorkspace=peaks, PointGroup='-31m (Trigonal - Hexagonal)')
    peak = pw.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()

Output:

.. testoutput:: ExSortHKLOption

    HKL of first peak in table 1 4.0 -9.0
    HKL of first peak in table 1 3.0 -8.0


.. categories::
