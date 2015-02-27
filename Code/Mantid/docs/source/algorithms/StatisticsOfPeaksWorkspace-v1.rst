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
    
    pw,stats = StatisticsOfPeaksWorkspace(InputWorkspace=peaks, PointGroup='-31m (Trigonal - Rhombohedral)', SortBy="Overall")
    peak = pw.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()
    print stats.row(0)

Output:

.. testoutput:: ExStatisticsOfPeaksWorkspaceOption

    HKL of first peak in table 1 4.0 -9.0
    HKL of first peak in table -10 3.0 -40.0
    {'Data Completeness': inf, 'Rmerge': 1.0230738280504019, 'Multiplicity': 1.0074257425742574, 'Resolution Min': 0.29574100000000003, 'No. of Unique Reflections': 405, 'Mean ((I)/sd(I))': 27.50726166791943, 'Resolution Max': 3.1660760000000003, 'Resolution Shell': 'Overall', 'Rpim': 1.0230738280504019}


.. categories::
