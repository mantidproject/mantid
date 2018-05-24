.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

SortHKL calculates some data set statistics using the intensities of the peaks in the
supplied input workspace, such as :math:`R_{merge}` and :math:`R_{p.i.m.}`, (formulas
and references for example `here <http://strucbio.biologie.uni-konstanz.de/ccp4wiki/index.php/R-factors>`_)
but also redundancy and completeness.

At first, the algorithm determines the minimum and maximum :math:`d`-value from the
input peaks. Using these limits, the unit cell, centering and point group all possible
unique reflections are calculated, the number of which is :math:`N_{theor.}`.

After removing invalid peaks with :math:`I \leq 0`, :math:`\sigma \leq 0` and :math:`h=k=l=0`,
the peaks are assigned to their respective unique reflection so that each theoretically present
reflection may have :math:`n` observations (:math:`n` can be zero). The number of unique reflections
which have at least one observation can be labeled :math:`N_{unique}`. The completeness
is defined as the fraction :math:`\frac{N_{unique}}{N_{theor.}}` and ranges between 0 and 1.
The total number of observations :math:`N_{observed}` and :math:`N_{unique}` determine the average
redundancy :math:`\frac{N_{observed}}{N_{unique}}` in the data set, which is the average number of
observations for each unique reflection.

The intensities of peaks in each reflection are checked for outliers, which are removed. Outliers
in this context are peaks with an intensity that deviates more than :math:`3\sigma_{hkl}` from the
mean of the reflection, where :math:`\sigma_{hkl}` is the standard deviation of the peak intensities.

The intensities and errors of each peak is set to the mean intensity and sigma of the unique reflection
it belongs to, so that equivalent reflections have the same intensity and error in the output workspace.

Finally, the peaks in the output workspace are sorted by H, K and L.

The EquivalentsWorkspace contains specta that can be plotted for each set of
equivalent intensities.  The X axis is the wavelength and the Y axis is the corrected intensity of the
peaks.  The error is the difference in the intensity of that peak and the average for all equivalent
peaks.  For example, see the 424 equivalent intensities in plot below.  The intensity of the peak at 
wavelength 0.5 is marked as an outlier by setting the error to the same value as the intensity. 
The average intensity is 21903.

.. figure:: /images/EquivalentIntensities.png

Usage
-----

The following usage example uses data obtained from a trigonal structure. The peaks are loaded and a :ref:`UB matrix <Lattice>`
is determined. The peaks are transformed to conform with a conventional cell, which has hexagonal metric
and rhombohedral centering:

.. testcode:: ExSortHKLOption

    # Load example peak data and find cell
    peaks = LoadIsawPeaks(Filename=r'Peaks5637.integrate')

    FindUBUsingFFT(peaks, MinD=0.25, MaxD=10, Tolerance=0.2)
    SelectCellWithForm(peaks, FormNumber=9, Apply=True, Tolerance=0.15)
    OptimizeLatticeForCellType(peaks,
                               CellType='Hexagonal', Apply=True, Tolerance=0.2)

    # Run the SortHKL algorithm
    sorted, chi2, statistics_table, equivI = SortHKL(peaks, PointGroup='-3m1 (Trigonal - Hexagonal)',
                                             LatticeCentering='Rhombohedrally centred, obverse')

    statistics = statistics_table.row(0)

    print('Data set statistics:')
    print('        Peaks: {0}'.format(sorted.getNumberPeaks()))
    print('       Unique: {0}'.format(statistics['No. of Unique Reflections']))
    print(' Completeness: {0}%'.format(round(statistics['Data Completeness'], 2)))
    print('   Redundancy: {0}'.format(round(statistics['Multiplicity'], 2)))

Output:

.. testoutput:: ExSortHKLOption

    Data set statistics:
            Peaks: 408
           Unique: 337
     Completeness: 9.11%
       Redundancy: 1.21

.. categories::

.. sourcelink::
