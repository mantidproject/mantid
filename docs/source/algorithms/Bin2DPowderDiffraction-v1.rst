
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs binning of the given workspace in :math:`(d, d_{\perp})` space, where :math:`d` is the d-Spacing and

.. math::
   :label:

    d_{\perp} = \sqrt{\lambda^2 - 2\log\cos\theta}

The result is that a Matrix workspace is created. If :math:`\theta=0` or :math:`\cos\theta\le 0`, the algorithm will terminate with an
error message, since no reasonable :math:`d` or :math:`d_{\perp}` value can be calculated. In this case it is recommended to check
whether detector positions are correct and to mask the problem detectors.


.. warning::

   The information about correspondence of spectra to detectors is lost in the produced **OutputWorkspace**.
   Applying any algorithms like :ref:`algm-ConvertUnits`, which require information about :math:`2\theta`,
   to this workspace may lead to incorrect results.


Restrictions on the input workspace
###################################

-  X-axis must have the wavelength units.
-  Only Histograms can be handled.
-  Only :ref:`EventWorkspaces <EventWorkspace>` are supported for the moment.
-  The input workspace must have an instrument set.
-  The input workspace must have a Spectrum axis as a Y-axis.


Binning parameters
------------------
Either *dSpaceBinning* and *dPerpendicularBinning* or *BinEdgesFile* must be specified, but not both. *dSpaceBinning* contains binning parameters
in d-Spacing. *dPerpendicularBinning* contains binning parameters in d-SpacingPerpendicular. Binning parameters should be set the same way as
for :ref:`algm-Rebin` algorithm.

In the case if non-equidistant binning is required, bin edges can be specified in the *BinEdgesFile*.

BinEdgesFile
############

*BinEdgesFile* is an ascii-file, where the bin edges are specified in a following format.

The first 2 lines contain header:

::

    #dp_min #dp_max
    #d_bins

Then follows the line dp = :math:`d_{\perp\,n}`  :math:`d_{\perp\,n+1}` where the bin edges for the *n*-th bin are specified,
starting from the lowest values. In the next line bin edges for d-Spacing are listed as :math:`d_0, d_1, \dots, d_m`. Then this
can be repeated as many times as necessary. For example:

::

    #dp_min #dp_max
    #d_bins
    dp = 3.0  4.0
        1.0  3.0  6.0

    dp = 4.0  4.5
        2.0  4.0  5.15  6.0

In this example the following bin edges are specified for the :math:`d_{\perp}`-axis: 3.0, 4.0, 4.5. The *d*-axis for the first spectrum
(:math:`d_{\perp}` from 3.0 to 4.0) will contain the bin edges 1.0, 3.0, 6.0 and for the second spectrum (:math:`d_{\perp}` from 4.0 to 4.5)
the bin edges 2.0, 4.0, 5.15, 6.0.



Usage
-----

**Example - Bin2DPowderDiffraction**

.. testcode:: Bin2DPowderDiffractionExample

   # Create an input workspace
   wsIn = CreateSampleWorkspace(WorkspaceType="Event", Function="Powder Diffraction",
                                NumBanks=1, XUnit="Wavelength", NumEvents=10,
                                XMin=1.0, XMax=6.0, BinWidth=1.0)
   # Move detector to get reasonable 2theta
   MoveInstrumentComponent(wsIn, 'bank1', X=1,Y=0,Z=1, RelativePosition=False)

   # Do binning
   wsOut = Bin2DPowderDiffraction(wsIn, dSpaceBinning="2,2,6", dPerpendicularBinning="1,2,5", NormalizeByBinArea=False)

   # Do binning and normalize the result by bin area
   wsOutNorm = Bin2DPowderDiffraction(wsIn, dSpaceBinning="2,2,6", dPerpendicularBinning="1,2,5", NormalizeByBinArea=True)

   # Print the result
   print("Y values without normalization:")
   print(wsOut.extractY())
   print("Y values with normalization by bin area:")
   print(wsOutNorm.extractY())

Output:

.. testoutput:: Bin2DPowderDiffractionExample
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

    Y values without normalization:
    [[... ...]
     [ ... ...]]
    Y values with normalization by bin area:
    [[... ...]
     [ ... ...]]


References
----------

More details on the multidimensional TOF powder diffraction data reduction can be found in the following papers.

#. P. Jacobs, A. Houben, W. Schweika, A. L. Tchougréeff and R. Dronskowski, *A Rietveld refinement method
   for angular- and wavelength-dispersive neutron time-of-flight powder diffraction data*, J. Appl. Cryst. (2015) 48, 1627-1636
   `doi: 10.1107/S1600576715016520 <https://doi.org/10.1107/S1600576715016520>`_
#. P. Jacobs, A. Houben, W. Schweika, A. L. Tchougréeff and R. Dronskowski, *Instrumental resolution as a function
   of scattering angle and wavelength as exemplified for the POWGEN instrument*, J. Appl. Cryst. (2017) 50, 866-875.
   `doi: 10.1107/S1600576717005398 <https://doi.org/10.1107/S1600576717005398>`_


.. categories::

.. sourcelink::

