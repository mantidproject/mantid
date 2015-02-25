.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Version two of PoldiFitPeaks1D was introduced to solve problems with overlapping peaks. Instead of refining individual peaks, the algorithm defines a range of x-values around each peak. In a second step the algorithm checks whether neighboring ranges are overlapping and merges them if that's the case. The amount of overlap that should be tolerated by the algorithm can be changed through the parameter `AllowedOverlap`, where 0 means that ranges that overlap in any way should be merged, while with 1, they are not merged at all. Merged ranges then contain more than one peak, which are refined together. This way, reasonable results can also be obtained for overlapping peaks.

Another thing that's different from version 1 of the algorithm is the background description. When peaks overlap, the description with a quadratic function is not particularly suitable. Instead, Chebyshev-polynomes of degrees 0, 1 and 2 are fitted and the best solution (with respect to quality of the fit) is selected automatically.

Usage
-----

.. include:: ../usagedata-note.txt

The following usage example loads an example correlation spectrum that was calculated for a sample containing two compounds with very similar lattice parameters.

.. testcode:: ExSiliconPeakFit2

    # Load correlation spectrum
    Load(Filename='poldi_2_phases_theoretical_reference.nxs', OutputWorkspace='correlation_spectrum')
    
    # Perform peak search
    PoldiPeakSearch(InputWorkspace='correlation_spectrum', MinimumPeakSeparation=8, MaximumPeakNumber=12, MinimumPeakHeight=180, OutputWorkspace='peaks')
    
    # Fit peaks with proper overlap handling
    PoldiFitPeaks1D(InputWorkspace='correlation_spectrum', FwhmMultiples=2, AllowedOverlap=0.1, PoldiPeakTable='peaks')
    
Variation of the `AllowedOverlap`-parameter influences the quality of the fit at some point if it's too close to 1. Setting it to 1 makes the algorithm's behavior similar to that of version 1.

.. testcode:: ExSiliconPeakFit2

    # Load correlation spectrum
    Load(Filename='poldi_2_phases_theoretical_reference.nxs', OutputWorkspace='correlation_spectrum')
    
    # Perform peak search
    PoldiPeakSearch(InputWorkspace='correlation_spectrum', MinimumPeakSeparation=8, MaximumPeakNumber=9, MinimumPeakHeight=180, OutputWorkspace='peaks')
    
    # Too large allowed overlap, fits will have bad quality.
    PoldiFitPeaks1D(InputWorkspace='correlation_spectrum', FwhmMultiples=2, AllowedOverlap=0.9, PoldiPeakTable='peaks')
    
.. categories::
