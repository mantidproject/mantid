
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

The resulting calibrated geometry can be exported by
:ref:`algm-ExportGeometry`.

ComponentList
#############

The *ComponentList* can be any instrument component that can be moved
and rotated. For example in POWGEN you can list *bank46* or *Column4*
(which includes banks 42-46) or *Group3* (which all the banks in
Column 3 and 4). In the case of a component group it is treated as one
object and not individual banks. You can list multiple components
which will be refined in turn (*e.g.* in the *Align the Y rotation of
bank26 and bank46 in POWGEN* usage example below).

Masking
#######

The only masking that is on taken into account when minimising the
difference in *DIFC* is the masking in the workspace of the
MaskWorkspace property of AlignComponents.

Fitting Sample/Source
#####################

When fitting the sample or source position it uses the entire
instrument and only moves the *Z* position (*i.e.* along the
beam). You can use a masking workspace to mask part of the instrument
you don't want to use to align the sample/source position (*e.g.* in
the *Align sample position in POWGEN* usage example below).

The source and sample positions (in that order) are aligned before an
components are aligned.

Usage
-----

**Example - Align the X and Z position of bank26 in POWGEN:**

.. testcode:: position

      LoadCalFile(InstrumentName="PG3",
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=True,
            WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      component="bank26"
      print "Start position is",ws.getInstrument().getComponentByName(component).getPos()
      AlignComponents(CalibrationTable="PG3_cal",
              Workspace=ws,
	      MaskWorkspace="PG3_mask",
	      Xposition=True, ZPosition=True,
              ComponentList=component)
      ws=mtd['ws']
      print "Final position is",ws.getInstrument().getComponentByName(component).getPos()

Output:

.. testoutput:: position

    Start position is [1.54436,0.863271,-1.9297]
    Final position is [1.50591,0.863271,-1.92734]

**Example - Align the Y rotation of bank26 and bank46 in POWGEN:**

.. testcode:: rotation

      LoadCalFile(InstrumentName="PG3",
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=True,
	    WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      components="bank26,bank46"
      print "Start bank26 rotation is",ws.getInstrument().getComponentByName("bank26").getRotation().getEulerAngles()
      print "Start bank46 rotation is",ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
      AlignComponents(CalibrationTable="PG3_cal",
	      Workspace=ws,
	      MaskWorkspace="PG3_mask",
	      EulerConvention="YZX",
              alphaRotation=True,
	      ComponentList=components)
      ws=mtd['ws']
      print "Final bank26 rotation is",ws.getInstrument().getComponentByName("bank26").getRotation().getEulerAngles()
      print "Final bank46 rotation is",ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()

.. testoutput:: rotation

      Start bank26 rotation is [-24.0613,0.120403,18.0162]
      Start bank46 rotation is [-41.0917,0.060773,17.7948]
      Final bank26 rotation is [-25.2256,0.120403,18.0162]
      Final bank46 rotation is [-37.3972,0.060773,17.7948]

**Example - Align sample position in POWGEN:**

.. testcode:: sample

      LoadCalFile(InstrumentName="PG3",
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=True,
	    WorkspaceName="PG3")
      # Mask banks that don't have calibration data
      MaskBTP(Workspace='PG3_mask', Instrument='POWGEN',
	      Bank='22-25,42-45,62-66,82-86,102-105,123,124,143,144,164,184,204')
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      print "Start sample position is",ws.getInstrument().getSample().getPos().getZ()
      AlignComponents(CalibrationTable="PG3_cal",
            Workspace=ws,
            MaskWorkspace="PG3_mask",
            FitSamplePosition=True)
      print "Final sample position is",mtd['ws'].getInstrument().getSample().getPos().getZ()

.. testoutput:: sample

      Start sample position is 0.0
      Final sample position is 0.028259327914

.. categories::

.. sourcelink::
