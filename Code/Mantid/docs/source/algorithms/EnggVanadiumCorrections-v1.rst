.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is being developed for a specific instrument and
   technique. It might get changed or even removed without a
   notification, should instrument scientists decide to do so.

This algorithm performs calculations related to two types of
corrections with respect to a reference Vanadium diffraction dataset
(workspace), in the following order:

1. sensitivity correction
2. pixel-by-pixel correction

The algorithm outputs the features extracted from the Vanadium data
that are used to perform the two types of corrections, respectively:

1. the integration of every individual Vanadium spectrum
2. a list of curves of aggregated counts as a function of
   time-of-flight (one per bank)

These outputs can be used to apply the corrections on a diffraction
data workspace in the same algorithm run, or be used subsequently to
apply corrections to different input workspaces. In practice, both
outputs need to be calculated only once for every reference Vanadium
dataset, while they would normally be used to correct a series of
different diffraction data workspaces.

If no input/output workspace with diffraction data is passed, the
algorithm will calculate features that can then be used to apply
Vanadium corrections in other *Engg* algorithms. These corrections
will need to be applied in subsequent runs of the algorithm, but the
same correction features (integration and curves) can be re-used for
as long as the same reference Vanadium diffraction data is used.

If an input/output workspace with diffraction data is passed, the
algorithm applies the corrections on the diffraction data workspace by
using the integration and curves from a reference Vanadium datasset.

Normally this algorithm can be used in two different ways:
1. Pre-calculate correction features from a Vanadium data workspace.
2. Apply Vanadium corrections once the correction features have been
   calculated.

Examples of these two alternatives are shown below. In the first
option, only the input VanadiumWorkspace is required, and the two
outputs are produced normally. Optionally, a diffraction data
workspace can be passed in the input property InputWorkspace for it to
be corrected. In the second option, the corrections can be applied by
using pre-calculated features from a previous run of this algorithm
(both IntegrationWorkspace and CurvesWorkspace have to be passed as
input properties, If these two properties are not passed, they will be
re-calculated provided that a VanadiumWorkspace is passed which is not
recommended).

alternatively but not recommended if it can be avoided, calculating
the correction features from the Vanadium diffraction data.

This algorithm is used as a child algorithm in the algorithms
:ref:`algm-EnggFocus` and :ref:`algm-EnggCalibrateFull`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - apply Vanadium corrections on a sample workspace from an EnginX run file:**

.. testcode:: ExSimpleVanadiumCorr

   # # To generate the pre-calculated features (integration and curves), for a new
   # Vanadium run, and apply the corrections on a workspace:
   #             
   # data_ws = Load('ENGINX00213855.nxs')
   # van_ws = Load('ENGINX00236516.nxs')
   # EnggVanadiumCorrections(Workspace = data_ws, VanadiumWorkspace = van_ws
   #                         IntegrationWorkspace = 'integ_ws',
   #                         CurvesWorkspace = 'curves_ws')
   #
   # # Now you can save the two pre-calculated features / workspaces:
   # SaveNexus(InputWorkspace='integ_ws',
   #           Filename='ENGINX_precalculated_vanadium_run000236516_integration.nxs',)
   # SaveNexus(InputWorkspace='curves_ws',
   #           Filename='ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')
   #
   # # (not done here because the Vanadium run file has a large number of events)              
   #
   # # Below we use the pre-calculated features that can be obtained with
   # # the commands listed above.

   sample_ws = Load('ENGINX00213855.nxs')
   integ_ws = LoadNexus('ENGINX_precalculated_vanadium_run000236516_integration.nxs')
   curves_ws = LoadNexus('ENGINX_precalculated_vanadium_run000236516_bank_curves.nxs')
   corr_ws =  EnggVanadiumCorrections(Workspace = sample_ws,
                                      IntegrationWorkspace = integ_ws,
                                      CurvesWorkspace = curves_ws)

   # Should have one spectrum only
   print "No. of spectra:", focussed_ws.getNumberHistograms()

   # Print a few arbitrary bins where higher intensities are expected
   fmt = "For TOF of {0:.3f} intensity is {1:.3f}"
   for bin in [3169, 6037, 7124]:
     print fmt.format(focussed_ws.readX(0)[bin], focussed_ws.readY(0)[bin])

.. testcleanup:: ExVanadiumCorr

   DeleteWorkspace(sample_ws)
   DeleteWorkspace(integ_ws)
   DeleteWorkspace(curves_ws)
   DeleteWorkspace(corr_ws)

Output:

.. testoutput:: ExVanadiumCorr

   No. of spectra: 1
   For TOF of 20165.642 intensity is 13.102
   For TOF of 33547.826 intensity is 17.844
   For TOF of 38619.804 intensity is 32.768
   
.. categories::

.. sourcelink::
