.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calibrates panels of Rectangular Detectors 
or packs of tubes in an instrument.  The initial path,
panel centers and orientations are adjusted so the error in Q
positions from the theoretical Q positions is minimized. 
Given a set of peaks indexed by :math:`(h_i, k_i, l_i)`, we
modify the instrument parameters, p, and then find  Q in the sample frame,
:math:`\rm Q_{sample}` that mininizes the following:

.. math::

   \left\vert 2\pi \rm U \rm B \left(
                               \begin{array}{c}
                                 NINT(h_i) \\
                                 NINT(k_i) \\
                                 NINT(l_i) \\
                               \end{array}
                             \right) - \rm Q_{sample,i}(p) \right\vert ^2

NINT is the nearest integer function.
B is fixed from the input lattice parameters, but U is modified by :ref:`CalculateUMatrix <algm-CalculateUMatrix>` 
for all peaks before and after optimization.
When the peaks are indexed, sample offsets are adjusted to better index the peaks. 
The initial time-of-flight, T0, is optimized for all peaks before any parameters are optimized. 
The initial path, L1, is optimized for all peaks before and after all panels or packs' parameters are optimized.
The panels and packs' parameters are optimized in parallel.
An option is available to adjust the panel widths and heights for Rectangular Detectors in a second iteration with all the other parameters fixed.

OUTPUT workspaces and files:
############################

1) The results are saved to an ISAW-like DetCal file and optionally in an xml
   file that can be used with the :ref:`LoadParameterFile <algm-LoadParameterFile>` algorithm.

2) There are two output workspace groups that are output when this algorithm calls the :ref:`Fit <algm-Fit>` algorithm.

   a. Fit_Parameters: workspaces beginning with 'params' contains the results from fitting for each bank and for L1.

      * XShift, YShift,and ZShift are in meters.

      * XRotate, YRotate, and ZRotate are in degrees. 

   b. Fit_Residuals: workspaces beginning with 'fit' contain the differences in the calculated and theoretical Q vectors for each peak.
      
3) There are three output files that show the goodness of the calibration.

   a. ColFilename contains the calculated and theoretical column for each peak. Each spectra is labeled by the bank. To plot use python script, scripts/SCD_Reduction/SCDCalibratePanelsResults.py

   b. RowFilename contains the calculated and theoretical row for each peak. Each spectra is labeled by the bank. To plot use python script, scripts/SCD_Reduction/SCDCalibratePanelsResults.py

   c. TofFilename contains the calculated and theoretical TOF for each peak.  Each spectra is labeled by the bank.



After Calibration
-----------------

After calibration, you can save the workspace to Nexus (or Nexus
processed) and get it back by loading in a later Mantid session. You can
copy the calibration to another workspace using the same instrument by
means of the :ref:`algm-CopyInstrumentParameters`
algorithm. To do so select the workspace, which you have calibrated as
the InputWorkspace and the workspace you want to copy the calibration
to, the OutputWorkspace.

Usage
-----

.. testcode:: SCDCalibratePanels

    #Calibrate peaks file and load to workspace
    LoadIsawPeaks(Filename='MANDI_801.peaks', OutputWorkspace='peaks')
    #TimeOffset is not stored in xml file, so use DetCal output if you need TimeOffset
    SCDCalibratePanels(PeakWorkspace='peaks',DetCalFilename='mandi_801.DetCal',XmlFilename='mandi_801.xml',a=74,b=74.5,c=99.9,alpha=90,beta=90,gamma=60)
    LoadEmptyInstrument(Filename=config.getInstrumentDirectory() + 'MANDI_Definition_2013_08_01.xml', OutputWorkspace='MANDI_801_event_DetCal')
    CloneWorkspace(InputWorkspace='MANDI_801_event_DetCal', OutputWorkspace='MANDI_801_event_xml')
    LoadParameterFile(Workspace='MANDI_801_event_xml', Filename='mandi_801.xml')
    LoadIsawDetCal(InputWorkspace='MANDI_801_event_DetCal', Filename='mandi_801.DetCal')
    det1 = mtd['MANDI_801_event_DetCal'].getInstrument().getDetector(327680)
    det2 = mtd['MANDI_801_event_xml'].getInstrument().getDetector(327680)
    if det1.getPos() == det2.getPos():
        print("matches")
    
.. testcleanup:: SCDCalibratePanels

   DeleteWorkspace('peaks')
   DeleteWorkspace('MANDI_801_event_xml')
   DeleteWorkspace('MANDI_801_event_DetCal')
   import os,mantid   
   filename=mantid.config.getString("defaultsave.directory")+"mandi_801.xml"
   os.remove(filename)
   filename=mantid.config.getString("defaultsave.directory")+"mandi_801.DetCal"
   os.remove(filename)

Output:

.. testoutput:: SCDCalibratePanels

    matches
      
.. categories::

.. sourcelink::
