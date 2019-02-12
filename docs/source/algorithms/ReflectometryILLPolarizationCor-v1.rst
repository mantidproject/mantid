.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is an ILL specific wrapper around :ref:`PolarizationEffiencyCor <algm-PolarizationEfficiencyCor>`. It is typically run in reflectometry reduction workflow between :ref:`ReflectometryILLSumForeground <algm-ReflectometryILLSumForeground>` and :ref:`ReflectometryILLConvertToQ <algm-ReflectometryILLConvertToQ>`.

The algorithm accepts a list of workspace names as *InputWorkspaces*. Thus, the workspaces have to be in the analysis data service. One to four workspaces can be named. If only a single workspace is given, it is treated as the direct beam.

The algorithm loads the polarization efficiencies from *EfficiencyFile*. This file should be in the ILL format as :ref:`LoadILLPolarizationFactors <algm-LoadILLPolarizationFactors>` is used for loading.

The first workspace in *InputWorkspaces* is picked as a reference. The instrument configuration is extracted from the sample logs of this workspace. Further, this workspace is used as *WavelengthReference* for :ref:`LoadILLPolarizationFactors <algm-LoadILLPolarizationFactors>`. To make sure the wavelength axes of the rest of *InputWorkspace* comply, they are run through :ref:`RebinToWorkspace <algm-RebinToWorkspace>`, the reference workspace being *WorkspaceToMatch*.

Finally, the algorithm extracts the flipper configurations from the input workspaces, reorders the workspaces accordingly and calls :ref:`PolarizationEffiencyCor <algm-PolarizationEfficiencyCor>`. *OutputWorkspace* is the workspace group returned by :ref:`PolarizationEffiencyCor <algm-PolarizationEfficiencyCor>`.

The following diagram shows the workflow of this algorithm:

.. diagram:: ReflectometryILLPolarizationCor-v1_wkflw.dot

Usage
-----

**Example - analyzerless corrections**

.. testcode:: AnalyzerlessEx

   # Use same foreground and background settings for direct and reflected
   # beams.
   # Python dictionaries can be passed to algorithms as 'keyword arguments'.
   settings = {
       'ForegroundHalfWidth':[5],
       'LowAngleBkgOffset': 10,
       'LowAngleBkgWidth': 20,
       'HighAngleBkgOffset': 10,
       'HighAngleBkgWidth': 50,
   }
   
   # Direct beam
   direct = ReflectometryILLPreprocess(
       Run='ILL/D17/317369.nxs',
       **settings
   )

   directFgd = ReflectometryILLSumForeground(direct)
   
   # Reflected beam. Flippers set to '++'
   reflected11 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       DirectLineWorkspace=direct,
       **settings
   )
   
   reflectivity11 = ReflectometryILLSumForeground(
       InputWorkspace=reflected11,
       DirectForegroundWorkspace=directFgd,
       DirectLineWorkspace=direct,
       WavelengthRange=[2, 15],
   )
   # Reload the reflected be. We will fake the '--' flipper settings
   reflected00 = ReflectometryILLPreprocess(
       Run='ILL/D17/317370.nxs',
       **settings
   )
   
   reflectivity00 = ReflectometryILLSumForeground(
       InputWorkspace=reflected00,
       DirectForegroundWorkspace=directFgd,
       DirectLineWorkspace=direct,
       WavelengthRange=[2, 15],
   )
   # Overwrite sample logs
   replace = True
   logs = reflectivity00.mutableRun()
   logs.addProperty('Flipper1.state', '-', replace)
   logs.addProperty('Flipper1.stateint', 0, replace)
   logs.addProperty('Flipper2.state', '-', replace)
   logs.addProperty('Flipper2.stateint', 0, replace)
   
   # Polarization efficiency correction
   # The algorithm will think that the analyzer was off.
   ReflectometryILLPolarizationCor(
       InputWorkspaces='reflectivity00, reflectivity11',
       OutputWorkspace='pol_corrected',  # Name of the group workspace
       EfficiencyFile='ILL/D17/PolarizationFactors.txt'
   )
   # The polarization corrected workspaces get automatically generated names
   polcorr00 = mtd['pol_corrected_--']
   polcorr11 = mtd['pol_corrected_++']
   # The output is almost the same as from ReflectometryILLSumForeground
   # except for small difference due to the polarization corrections.
   print('Histograms in 00 workspace: {}'.format(polcorr00.getNumberHistograms()))
   print('Histograms in 11 workspace: {}'.format(polcorr11.getNumberHistograms()))
   print('X unit: ' + polcorr00.getAxis(0).getUnit().unitID())

Output:

.. testoutput:: AnalyzerlessEx

   Histograms in 00 workspace: 1
   Histograms in 11 workspace: 1
   X unit: Wavelength

.. categories::

.. sourcelink::
