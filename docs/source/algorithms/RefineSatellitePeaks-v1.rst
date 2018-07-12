.. algorithm::

.. summary::

.. properties::

Description
-----------

RefineSatellitePeaks can be used to refine the locations of "satellite" peaks
occurring at fractional HKL locations in reciprocal space. RefineSatellitePeaks
takes a :ref:`MDWorkspace <MDWorkspace>` of experimental data, a
:ref:`PeaksWorkspace <PeaksWorkspace>` containing the locations of peaks with
integer HKL, and another PeaksWorkspace containing a subset of peaks found at
fractional HKL. 

.. figure:: /images/RefineSatellitePeaks-satellites.png
   :align: center
   :width: 600px
   :alt: RefineSatellitePeaks-satellites.png

   Satellite peaks exist at fractional HKL coordinates. The vector `q` is the
   Euclidean distance from the nearest integer HKL peak to the satellite. A
   schematic of this is shown in the HKL plane in the diagram above.

For each satellite peak the euclidean distance between the nearest integer peak
and satellite are computed in the HKL frame. Peaks are then grouped according
to euclidean distance using using the properties `NumOfQs` and
`ClusterThreshold`. If `NumOfQs` is specified then each offset will be grouped
into exactly `k` clusters. If `ClusterThreshold` is specified then offsets will
be grouped into clusters seperated by no more than a cophenetic distance below
this threshold.  The centroid of each cluster calculated for each group and is
used as the offset to predict the location of fractional peaks everywhere in
the :ref:`MDWorkspace <MDWorkspace>`.

.. figure:: /images/RefineSatellitePeaks-clustering.png
   :align: center
   :width: 700px
   :alt: RefineSatellitePeaks-clustering.png

   Due to noise in the experimental data the `q` vector for every satellite may
   vary slightly. This algorithm calculates the `q` vectors for all peaks passed
   as starting points into distinct sets of `q` vectors. The centroid of each
   cluster is then taken as the "true" value of `q` and is used to predict the
   postion of all other fractional peaks with this `q`.
   

For each predicted fractional peak, the local centroid (the radius of which is
defined by `PeakRadius`) in the MD data is found and this is taken as the actual
position of the satellite peaks.

Finally the found satellite peaks are integerated using the
:ref:`IntegratePeaksMD <algm-IntegratePeaksMD-v2>` algorithm with the parameters `PeakRadius`,
`BackgroundInnerRadius`, and `BackgroundOuterRadius`. Satellite peaks are
discarded if there I/sigma value is less than the parameter `IOverSigma`.

.. figure:: /images/RefineSatellitePeaks-centroids.png
   :align: center
   :width: 700px
   :alt: RefineSatellitePeaks-centroid.png

   Finally, using the predicted satellite peak positions (green) the local
   centroid is found and this is used as the true position of the experimental
   satellite peak (orange). The area to search for the centroid is controlled by
   the `PeakRadius` parameter. All centroids are integrated a filtered by
   intensity and :math:`\frac{I}{\sigma}`. If the experimental satellite has
   zero intesity or is below the :math:`\frac{I}{\sigma}` threshold then it is
   discarded.


The output of the algorithm is a peaks workspace containing all of the
satellite peaks found by the algorithm in fractional coordinates. Each of the
peaks will have an intensity value from the :ref:`IntegratePeaksMD
<algm-IntegratePeaksMD-v2>` algorithm. 

.. warning:: This integration is very approximate and may not produce the best
    possible values. It is recommended that satellite peaks are reintegrated
    using one of the many peak integration algorithms and to tune their
    parameters to obtain the best possible peak integration. 

For more information on superspace crystallography see:

- Van Smaalen, Sander. "An elementary introduction to superspace 
  crystallography." Zeitschrift für Kristallographie-Crystalline Materials 
  219, no. 11 (2004): 681-691. 

- Van Smaalen, Sander. "Incommensurate crystal structures." Crystallography 
  Reviews 4, no. 2 (1995): 79-202. 

Related Algorithms
------------------
- :ref:`PredictFractionalPeaks <algm-PredictFractionalPeaks-v1>` predicts the
  postion of fractional peaks given a :ref:`PeaksWorkspace <PeaksWorkspace>` of
  nuclear peaks and a set of HKL offsets.

- :ref:`CentroidPeaksMD <algm-CentroidPeaksMD-v2>` is used to find the local 
  centroid from a predicted peak position.

- :ref:`IntegratePeaksMD <algm-IntegratePeaksMD-v2>` is used to integrate
  satellite peaks to check if they actually exist at the predicted position.

- :ref:`FilterPeaks <algm-FilterPeaks-v1>` is used to remove peaks with zero
  intensity or a I/sigma below the desired threshold.

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


    satellites_refined = RefineSatellitePeaks(NuclearPeaks=main_peaks, SatellitePeaks=satellite_peaks, MDWorkspace=md_workspace, **params)

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
