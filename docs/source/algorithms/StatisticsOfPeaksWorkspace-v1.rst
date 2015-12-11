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

    # Load example peak data and find cell
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')

    FindUBUsingFFT(peaks, MinD=0.25, MaxD=10, Tolerance=0.2)
    SelectCellWithForm(peaks, FormNumber=9, Apply=True, Tolerance=0.15)
    OptimizeLatticeForCellType(peaks,
                               CellType='Hexagonal', Apply=True, Tolerance=0.2)

    # Run the SortHKL algorithm
    sorted, statistics_table = StatisticsOfPeaksWorkspace(peaks, PointGroup='-3m1 (Trigonal - Hexagonal)',
                                                          LatticeCentering='Rhombohedrally centred, obverse',
                                                          SortBy='Overall')

    statistics = statistics_table.row(0)

    peak = sorted.getPeak(0)
    print "HKL of first peak in table %d" % peak.getH(),peak.getK(),peak.getL()
    print "Multiplicity = %.2f" % statistics['Multiplicity']
    print "Resolution Min = %.2f" % statistics['Resolution Min']
    print "Resolution Max = %.2f" % statistics['Resolution Max']
    print "No. of Unique Reflections = %i" % statistics['No. of Unique Reflections']
    print "Mean ((I)/sd(I)) = %.2f" % statistics['Mean ((I)/sd(I))']
    print "Rmerge = %.2f" % statistics['Rmerge']
    print "Rpim = %.2f" % statistics['Rpim']

Output:

.. testoutput:: ExStatisticsOfPeaksWorkspaceOption

    HKL of first peak in table -10 5.0 42.0
    Multiplicity = 1.21
    Resolution Min = 0.30
    Resolution Max = 3.17
    No. of Unique Reflections = 337
    Mean ((I)/sd(I)) = 27.51
    Rmerge = 10.08
    Rpim = 10.08


.. categories::

.. sourcelink::
