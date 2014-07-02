.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs integration of single-crystal peaks within a
radius (with optional background subtraction) in reciprocal space.

Inputs
######

The algorithms takes two input workspaces:

-  A MDEventWorkspace containing the events in multi-dimensional space.
   This would be the output of
   :ref:`algm-ConvertToDiffractionMDWorkspace`.
-  As well as a PeaksWorkspace containing single-crystal peak locations.
   This could be the output of :ref:`algm-FindPeaksMD`
-  The OutputWorkspace will contain a copy of the input PeaksWorkspace,
   with the integrated intensity and error found being filled in.

Calculations
############

Integration is performed by summing the weights of each MDEvent within
the provided radii. Errors are also summed in quadrature.

.. figure:: /images/IntegratePeaksMD_graph1.png
   :alt: IntegratePeaksMD_graph1.png

   IntegratePeaksMD\_graph1.png

-  All the Radii are specified in :math:`\AA^{-1}`
-  A sphere of radius **PeakRadius** is integrated around the center of
   each peak.

   -  This gives the summed intensity :math:`I_{peak}` and the summed
      squared error :math:`\sigma I_{peak}^2`.
   -  The volume of integration is :math:`V_{peak}`.

-  If **BackgroundOuterRadius** is specified, then a shell, with radius
   r where **BackgroundInnerRadius** < r < **BackgroundOuterRadius**, is
   integrated.

   -  This gives the summed intensity :math:`I_{shell}` and the summed
      squared error :math:`\sigma I_{shell}^2`.
   -  The volume of integration is :math:`V_{shell}`.
   -  **BackgroundInnerRadius** allows you to give some space between
      the peak and the background area.

Background Subtraction
######################

The background signal within PeakRadius is calculated by scaling the
background signal density in the shell to the volume of the peak:

:math:`I_{bg} = I_{shell} \frac{V_{peak}}{V_{shell}}`

with the error squared on that value:

:math:`\sigma I_{bg}^2 = \frac{V_{peak}}{V_{shell}} \sigma I_{shell}^2`

This is applied to the integrated peak intensity :math:`I_{peak}` to
give the corrected intensity :math:`I_{corr}`:

:math:`I_{corr} = I_{peak} - I_{bg}`

with the errors summed in quadrature:

:math:`\sigma I_{corr}^2 = \sigma I_{peak}^2 + \sigma I_{bg}^2`

If BackgroundInnerRadius is Omitted
###################################

If BackgroundInnerRadius is left blank, then **BackgroundInnerRadius** =
**PeakRadius**, and the integration is as follows:

.. figure:: /images/IntegratePeaksMD_graph2.png
   :alt: IntegratePeaksMD_graph2.png

   IntegratePeaksMD\_graph2.png

Sample Usage
############

.. code-block:: python

    # Load a SCD data set and find the peaks
    LoadEventNexus(Filename=r'TOPAZ_3131_event.nxs',OutputWorkspace='TOPAZ_3131_nxs')
    ConvertToDiffractionMDWorkspace(InputWorkspace='TOPAZ_3131_nxs',OutputWorkspace='TOPAZ_3131_md',LorentzCorrection='1')
    FindPeaksMD(InputWorkspace='TOPAZ_3131_md',PeakDistanceThreshold='0.15',MaxPeaks='100',OutputWorkspace='peaks')
    FindUBUsingFFT(PeaksWorkspace='peaks',MinD='2',MaxD='16')

    # Perform the peak integration, in-place in the 'peaks' workspace.
    IntegratePeaksMD(InputWorkspace='TOPAZ_3131_md', PeaksWorkspace='peaks',
        PeakRadius=0.12, BackgroundOuterRadius=0.2, BackgroundInnerRadius=0.16,
        OutputWorkspace='peaks')

.. categories::
