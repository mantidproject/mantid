.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

RefineSatellitePeaks can be used to refine the locations of "satellite" peaks
occurring at fractional HKL locations in reciprocal space. RefineSatellitePeaks
takes a :ref:`MDWorkspace <MDWorkspace>` of experimental data, a
:ref:`PeaksWorkspace <PeaksWorkspace>` containing the locations of peaks with
integer HKL, and another PeaksWorkspace containing a subset of peaks found at
fractional HKL. 

For each satellite peak the euclidean distance between the nearest integer peak
and satellite are computed in the HKL frame. Peaks are then grouped according
to euclidean distance using using the properties `NumOfQs` and
`ClusterThreshold`. If `NumOfQs` is specified then each offset will be grouped
into exactly `k` clusters. If `ClusterThreshold` is specified then offsets will
be grouped into clusters seperated by no more than a cophenetic distance below
this threshold.  The centroid of each cluster calculated for each group and is
used as the offset to predict the location of fractional peaks everywhere in
the :ref:`MDWorkspace <MDWorkspace>`.

For each predicted fractional peak, the local centroid (the radius of which is
defined by `PeakRadius`) in the MD data is found and this is taken as the actual
position of the satellite peaks.

Finally the found satellite peaks are integerated using the
:ref:`algm-IntegrateMD` algorithm with the parameters `PeakRadius`,
`BackgroundInnerRadius`, and `BackgroundOuterRadius`. Satellite peaks are
discarded if there I/sigma value is less than the parameter `IOverSigma`.

Usage
-----

.. include:: ../usagedata-note.txt


**Example - calling RefineSatellitePeaks:**

.. testcode:: RefineSatellitePeaks

    md_workspace = Load(Filename='WISH_md_small.nxs', OutputWorkspace='WISH_md_small')
    satellite_peaks = Load(Filename='WISH_peak_hkl_frac_small.nxs', OutputWorkspace='WISH_peak_hkl_frac_small')
    main_peaks = Load(Filename='WISH_peak_hkl_small.nxs',OutputWorkspace='WISH_peak_hkl_small')

    params = {
        "PeakRadius": 0.3,
        "BackgroundInnerRadius": 0.3,
        "BackgroundOuterRadius": 0.4,
        "OutputWorkspace": "refined_peaks",
        "NumOfQs": 2
    }


    satellites_refined = RefineSatellitePeaks(MainPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace, **params)

    print (len(satellites_refined))

.. testcleanup:: RefineSatellitePeaks

   DeleteWorkspace('WISH_md_small')
   DeleteWorkspace('WISH_peak_hkl_small')
   DeleteWorkspace('WISH_peak_hkl_frac_small')
   DeleteWorkspace('refined_peaks')

Output:

.. testoutput:: RefineSatellitePeaks

    4

.. categories::

.. sourcelink::
  :cpp: None
  :h: None
