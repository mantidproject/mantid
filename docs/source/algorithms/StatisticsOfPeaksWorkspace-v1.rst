.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Statistics of the Peaks Workspaces are calculated for all peaks and by
default for resolution shell(d-Spacing).  There is a SortBy option to change this
to by orientation (RunNumber) or by Anger camera (bank) or only do all peaks.
This algorithm calls :ref:`algm-SortHKL`, so more details are in the documentation for that algorithm.

After removing invalid peaks with :math:`I \leq 0`, :math:`\sigma \leq 0` and :math:`h=k=l=0`,
the peaks are assigned to their respective unique reflection so that each theoretically present
reflection may have :math:`n` observations (:math:`n` can be zero). The number of unique reflections
which have at least one observation can be labeled :math:`N_{unique}`.

Currently the satellite peaks are removed so only peaks with :math:`m=n=p=0` are used in the statistics.
In the future, this algorithm will also calculate statistics for these peaks.

The intensities of peaks in each reflection are checked for outliers, which are removed. Outliers
in this context are peaks with an intensity that deviates more than :math:`3\sigma_{hkl}` from the
mean of the reflection, where :math:`\sigma_{hkl}` is the standard deviation of the peak intensities.

Formulas for the statistics columns are:

        :math:`N_{unique}`

        :math:`dSpacing_{min}`

        :math:`dSpacing_{max}`

        :math:`Multiplicity =  \frac{N_{observed}}{N_{unique}}`

        :math:`\langle \frac{I}{\sigma_I} \rangle`

        In the following, all sums over hkl extend only over unique reflections with more than one observation! Output is percentages.

        :math:`R_{merge} = 100 * \frac{\sum_{hkl} \sum_{j} \vert I_{hkl,j}-\langle I_{hkl}\rangle\vert}{\sum_{hkl} \sum_{j}I_{hkl,j}}`
        where :math:`\langle I_{hkl}\rangle` is the average of j multiple measurements of the n equivalent reflections.


        :math:`R_{pim} = 100 * \frac{\sum_{hkl} \sqrt \frac{1}{n-1} \sum_{j=1}^{n} \vert I_{hkl,j}-\langle I_{hkl}\rangle\vert}{\sum_{hkl} \sum_{j}I_{hkl,j}}`

        :math:`Completeness =  100 * \frac{N_{unique}}{N_{theoretical}}`


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
    sorted, statistics_table, equivI = StatisticsOfPeaksWorkspace(peaks, PointGroup='-3m1 (Trigonal - Hexagonal)',
                                                          LatticeCentering='Rhombohedrally centred, obverse',
                                                          SortBy='Overall')

    statistics = statistics_table.row(0)

    peak = sorted.getPeak(0)
    print("HKL of first peak in table {} {} {}".format(peak.getH(),peak.getK(),peak.getL()))
    print("Multiplicity = %.2f" % statistics['Multiplicity'])
    print("Resolution Min = %.2f" % statistics['Resolution Min'])
    print("Resolution Max = %.2f" % statistics['Resolution Max'])
    print("No. of Unique Reflections = %i" % statistics['No. of Unique Reflections'])
    print("Mean ((I)/sd(I)) = %.2f" % statistics['Mean ((I)/sd(I))'])
    print("Rmerge = %.2f" % statistics['Rmerge'])
    print("Rpim = %.2f" % statistics['Rpim'])

Output:

.. testoutput:: ExStatisticsOfPeaksWorkspaceOption

    HKL of first peak in table -10.0 5.0 42.0
    Multiplicity = 1.21
    Resolution Min = 0.21
    Resolution Max = 2.08
    No. of Unique Reflections = 337
    Mean ((I)/sd(I)) = 27.51
    Rmerge = 10.08
    Rpim = 10.08


.. categories::

.. sourcelink::
