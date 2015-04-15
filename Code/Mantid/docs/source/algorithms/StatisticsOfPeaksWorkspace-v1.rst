.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Statistics of the Peaks Workspaces are calculated for all peaks and by
default for resolution shell.  There is a SortBy option to change this
to by orientation (RunNumber) or by Anger camera (bank) or only do all peaks.

Statistics include:
	No of Unique Reflections
	Resolution range
	Multiplicity
	Mean ((I)/sd(I))
	Rmerge
	Rplm
	Data Completeness


Usage
-----

**Example - an example of running StatisticsOfPeaksWorkspace with PointGroup option.**

.. testcode:: ExStatisticsOfPeaksWorkspaceOption

    #load a peaks workspace from file
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')
    LoadIsawUB(peaks,"ls5637.mat")
    peak = peaks.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()
    
    pw,stats = StatisticsOfPeaksWorkspace(InputWorkspace=peaks, PointGroup='-31m (Trigonal - Hexagonal)', SortBy="Overall")
    peak = pw.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()
    print "Rmerge = %.9f" % stats.row(0)['Rmerge']
    print "Multiplicity = %.9f" % stats.row(0)['Multiplicity']
    print "Resolution Min = %.9f" % stats.row(0)['Resolution Min']
    print "No. of Unique Reflections = %i" % stats.row(0)['No. of Unique Reflections']
    print "Mean ((I)/sd(I)) = %.9f" % stats.row(0)['Mean ((I)/sd(I))']
    print "Resolution Max = %.9f" % stats.row(0)['Resolution Max']
    print "Rpim = %.9f" % stats.row(0)['Rpim']

Output:

.. testoutput:: ExStatisticsOfPeaksWorkspaceOption

    HKL of first peak in table 1 4.0 -9.0
    HKL of first peak in table -10 3.0 -40.0
    Rmerge = 0.031365010
    Multiplicity = 1.012437811
    Resolution Min = 0.295741000
    No. of Unique Reflections = 403
    Mean ((I)/sd(I)) = 27.507261668
    Resolution Max = 3.166076000
    Rpim = 0.031365010


.. categories::
