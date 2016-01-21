
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will take the output of
:ref:`algm-GetDetOffsetsMultiPeaks` in the form of a
:ref:`diffractioncalibration
workspace<DiffractionCalibrationWorkspace>` and minimize the
difference between the DIFC of the instrument and calibration
workspace by moving and rotating instrument components.

The resulting caliibrated geometry can be exported by
:ref:`algm-ExportGeometry`.

Usage
-----

**Example - Align the X and Z position of bank26 in POWGEN:**

.. testcode:: position

      LoadCalFile(InstrumentName="PG3",
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=False,
            WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml")
      component="bank26"
      print "Start position is",ws.getInstrument().getComponentByName(component).getPos()
      AlignComponents(CalibrationTable="PG3_cal",
	      InputWorkspace=ws,
	      Zposition=False,
	      Xrotation=False, Yrotation=False, Zrotation=False,
              ComponentList=component)
      print "Final position is",ws.getInstrument().getComponentByName(component).getPos()

Output:

.. testoutput:: position

    Start position is [1.54436,0.863271,-1.9297]
    Final position is [1.53747,0.824442,-1.9297]

**Example - Align the Y rotaion of bank26 and bank46 in POWGEN:**

.. testcode:: rotation

      LoadCalFile(InstrumentName="PG3",
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=False,
	    WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml")
      components="bank26,bank46"
      print "Start bank26 rotation is",ws.getInstrument().getComponentByName("bank26").getRotation().getEulerAngles()
      print "Start bank46 rotation is",ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
      AlignComponents(CalibrationTable="PG3_cal",
	      InputWorkspace=ws,
	      Xposition=False, Yposition=False, Zposition=False,
              Xrotation=False, Zrotation=False,
	      ComponentList=components)
      print "Final bank26 rotation is",ws.getInstrument().getComponentByName("bank26").getRotation().getEulerAngles()
      print "Final bank46 rotation is",ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()

.. testoutput:: rotation

      Start bank26 rotation is [-24.0613,0.120403,18.0162]
      Start bank46 rotation is [-41.0917,0.060773,17.7948]
      Final bank26 rotation is [-24.0613,1.19777,18.0162]
      Final bank46 rotation is [-41.0917,2.40476,17.7948]

**Example - Align sample position in POWGEN:**

.. testcode:: sample

      LoadCalFile(InstrumentName="PG3",
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=False,
	    WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml")
      print "Start sample position is",ws.getInstrument().getSample().getPos().getZ()
      AlignComponents(CalibrationTable="PG3_cal",
	      InputWorkspace=ws,
	      Xposition=False, Yposition=False, Zposition=False,
              Xrotation=False, Yrotation=False, Zrotation=False)
      print "Final sample position is",ws.getInstrument().getSample().getPos().getZ()

.. testoutput::	sample

      Start sample position is 0.0
      Final sample position is 0.3

.. categories::

.. sourcelink::
