.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm calibrates sets of Rectangular Detectors in one
instrument. The initial path, time offset,panel widths, panel heights,
panel locations and orientation are all adjusted so the error in q
positions from the theoretical q positions is minimized. Also, there are
optimize options that take into account sample position and the need for
rigid rotations.

Some features:

1) Panels can be grouped.

| ``  All panels in a group will move the same way and rotate the same way.  If rigid rotations are``
| ``   used, each panel is rotated about the center of the instrument, along with panel pixels rotating``
| ``   around the panel's center. The height and  widths of the panels in a group will``
| ``    all change by the same factor``

2) The user can select which quantities to keep fixed during the
optimization.

3) The results can be saved to an ISAW-like DetCal file or in an xml
file that can be used with the LoadParameter algorithm.

4) Results from a previous optimization can be applied before another
optimization is done.

| ``  The Levenberg-Marquardt optimization algorithm is used. Later iterations may have too small of changes for the parameters to``
| ``  get to another optimum value.  Restarting allows for the consideration of parameter values further away and also can change``
| ``  constraints for the parameter values. This is also useful when fine tuning parameters that do not influence the errors as``
| ``  much as other parameters.``

5) There are several output tables indicating the results of the fit

| ``  A) ResultWorkspace contains the results from fitting.``
| ``    -t0 is in microseconds``
| ``    -L0 is in meters``
| ``    -*Xoffset,*Yoffset,and *Zoffset are in meters``
| ``    -*Xrot,*Yrot, and *Zrot are in degrees. Note that Zrot is done first, then Yrot , the Xrot.``

``  B)QErrorWorkspace contains the Error in Q values for each peak, along with other associated information about the peak``

``  C)CovarianceInfo contains the "correlations"(*100) between each of the parameters``

``6) Maximum changes in the quantities that are altered during optimization are now settable.``

"A" Workflow
------------

Optimizing all variables at once may not be the best option. The errors
become too large, so optimization in several stages subsets of the
variables are optimized at each stage.

First: NOTE that the input PeaksWorkspace does NOT CHANGE. This means
you should be able to keep trying different sets of variables until
things look good.

To work on another set of variables with the optimized first round of
optimized values

#. Use Preprocessinstrument to apply the previous DetCal or xml file
   before optimizing AND

#. Change the name of the target DetCal file, in case the choice of
   variables is not good. Then you will not clobber the good

DetCal file. AND

#. Change the name of the ResultWorkspace in the properties list. This
   means you will have a copy of the results from the

previous trial(s)( along with chiSq values) to compare results.

Do check the chiSquared values. If they do not decrease, you were close
to a minimum and the optimization could not get back to that minimum. It
makes a large jump at the beginning.

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
    SCDCalibratePanels(PeakWorkspace='peaks',DetCalFilename='mandi_801.DetCal',XmlFilename='mandi_801.xml',a=74,b=74.5,c=99.9,alpha=90,beta=90,gamma=60,usetimeOffset=False)
    LoadEmptyInstrument(Filename='MANDI_Definition_2013_08_01.xml', OutputWorkspace='MANDI_801_event_DetCal')
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
