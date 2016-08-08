.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calibrates panels of Rectangular Detectors in one
instrument. The initial path, panel widths, panel heights,
panel locations and orientation are all adjusted so the error in q
positions from the theoretical q positions is minimized. 

Some features:

1) The results can be saved to an ISAW-like DetCal file or in an xml
   file that can be used with the LoadParameter algorithm.

2) There are several output workspaces indicating the results of the fit

   a. Workspaces beginning with 'params' contains the results from fitting for each bank and for L1.

      * XShift, YShift,and ZShift are in meters.

      * XRotate, YRotate, and ZRotate are in degrees. 

   b. Workspaces beginning with 'fit' contain the differences in the calculated and theoretical Q vectors for each peak.
      
   c. ColFilename contains the calculated and theoretical column for each peak. Each spectra is labeled by the bank. To plot use python script, SCDCalibratePanelsResults.py

   d. RowFilename contains the calculated and theoretical row for each peak. Each spectra is labeled by the bank. To plot use python script, SCDCalibratePanelsResults.py

   e. TofFilename contains the calculated and theoretical TOF for each peak.  Each spectra is labeled by the bank.



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
        print "matches"
    
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
