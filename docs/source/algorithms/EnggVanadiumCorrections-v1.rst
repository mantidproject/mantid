.. algorithm::

.. summary::

.. relatedalgorithms::

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

1. the *integration* of every individual Vanadium spectrum
2. a list of *curves* of aggregated counts as a function of
   time-of-flight (one per bank)

If an input/output workspace with diffraction data is passed, the
algorithm applies the corrections on the diffraction data workspace by
using the integration and curves calculated from the reference
Vanadium datasset.

These outputs can be used to apply the corrections on a diffraction
data workspace in the same algorithm run, or be used subsequently to
apply corrections to different input workspaces. In practice, both
outputs need to be calculated only once for every reference Vanadium
dataset, while they would normally be used to correct a (possibly
long) series of different diffraction data workspaces.

If a vanadium data workspace is passed, the algorithm will calculate
features that can then be used to apply Vanadium corrections in other
*Engg* algorithms. If in addition an input/output workspace with
diffraction data is passed, the corrections will be applied on
it. Afterwards the same corrections can be applied on different
diffraction data workspaces by calling again this algorithm and
providing as inputs the integraion and curves workspaces produced by
the first call. The same correction features (integration and curves)
can be re-used for as long as the same reference Vanadium diffraction
data is still valid.

Normally this algorithm can be used in two different ways:

1. Pre-calculate correction features from a Vanadium data workspace.
2. Apply Vanadium corrections once the correction features have been
   calculated.

Examples of these two alternatives are shown below. In the first
option, only the input VanadiumWorkspace is required, and the two
outputs (integration and curves workspaces) are produced
normally. Optionally, a diffraction data workspace can be passed in
the input property InputWorkspace for it to be corrected. In the
second option, the corrections can be applied by using pre-calculated
features from a previous run of this algorithm (both
IntegrationWorkspace and CurvesWorkspace have to be passed as input
properties, If these two properties are not passed, they will be
re-calculated provided that a VanadiumWorkspace is passed which is not
recommended). All the calculations (integration, sums, divisions,
etc.) are done in the d-spacing space.

This algorithm is used as a child algorithm in the algorithms
:ref:`algm-EnggFocus` and :ref:`algm-EnggCalibrateFull`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - apply Vanadium corrections on a sample workspace from an EnginX run file:**

.. testcode:: ExVanadiumCorr

   # # To generate the pre-calculated features (integration and curves), for a new
   # Vanadium run, and apply the corrections on a workspace:
   #             
   # sample_ws = Load('ENGINX00213855.nxs')
   # van_ws = Load('ENGINX00236516.nxs')
   # EnggVanadiumCorrections(Workspace = sample_ws, VanadiumWorkspace = van_ws
   #                         OutIntegrationWorkspace = 'integ_ws',
   #                         OutCurvesWorkspace = 'curves_ws')
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
   EnggVanadiumCorrections(Workspace = sample_ws,
                           IntegrationWorkspace = integ_ws,
                           CurvesWorkspace = curves_ws)

   # Should have one spectrum only
   print("No. of spectra: {}".format(sample_ws.getNumberHistograms()))

   # Print a few arbitrary integrated spectra
   ws_idx = 400
   idx_count = 3
   integ_ws = Integration(sample_ws, StartWorkspaceIndex=ws_idx,
                          EndWorkspaceIndex=ws_idx+idx_count)
   fmt = "For workspace index {0:d} the spectrum integration is {1:.3f}"
   for i in range(idx_count):
      print(fmt.format(ws_idx+i, integ_ws.readY(i)[0]))

.. testcleanup:: ExVanadiumCorr

   DeleteWorkspace(sample_ws)
   DeleteWorkspace(integ_ws)
   DeleteWorkspace(curves_ws)

Output:

.. testoutput:: ExVanadiumCorr

   No. of spectra: 2513
   For workspace index 400 the spectrum integration is 23.998
   For workspace index 401 the spectrum integration is 23.799
   For workspace index 402 the spectrum integration is 22.872
   
.. categories::

.. sourcelink::
