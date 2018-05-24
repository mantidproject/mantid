.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will take a calibration in the form of a
:ref:`diffractioncalibration
workspace<DiffractionCalibrationWorkspace>` from the output of *for
example* :ref:`algm-GetDetOffsetsMultiPeaks` or
:ref:`algm-CalibrateRectangularDetectors` and minimize the difference
between the DIFC of the instrument and calibration workspace by moving
and rotating instrument components.

The resulting calibrated geometry can be exported by
:ref:`algm-ExportGeometry`.

ComponentList
#############

The *ComponentList* can include any instrument component that can be
moved and rotated by :ref:`algm-MoveInstrumentComponent` and
:ref:`algm-RotateInstrumentComponent`. For example in POWGEN you can
list *bank46* or *Column4* (which includes banks 42-46) or *Group3*
(which all the banks in Column 3 and 4). In the case of a component
group it is treated as one object and not individual banks. In some
instruments you can also specify individual tubes or pixel, *e.g.*
*bank20/tube3* and *bank20/tube3/pixel7*, although that is not the
intention of the algorithm. You can list multiple components which
will be refined in turn (*e.g.* in the *Align the Y rotation of bank26
and bank46 in POWGEN* usage example below).

Masking
#######

The only masking that is on taken into account when minimising the
difference in *DIFC* is the masking in the workspace of the
MaskWorkspace property of AlignComponents.

Fitting Sample/Source
#####################

When fitting the sample or source position it uses the entire
instrument and moves in the directions that you select. All rotation
options are ignored. You can use a masking workspace to mask part of
the instrument you don't want to use to align the sample/source
position (*e.g.* in the *Align sample position in POWGEN* usage
example below).

The source and sample positions (in that order) are aligned before an
components are aligned.

Usage
-----

**Example - Align the Y and Z position of bank26 in POWGEN:**

.. testcode:: position

      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      LoadCalFile(InputWorkspace=ws,
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=True,
            WorkspaceName="PG3")
      component="bank26"
      print("Start position is {}".format(ws.getInstrument().getComponentByName(component).getPos()))
      AlignComponents(CalibrationTable="PG3_cal",
              Workspace=ws,
	      MaskWorkspace="PG3_mask",
	      Yposition=True, ZPosition=True,
              ComponentList=component)
      ws=mtd['ws']
      final_pos = ws.getInstrument().getComponentByName(component).getPos()
      print("Final position is [{:.2f}.{:.2f},{:.2f}]".format(final_pos[0],final_pos[1],final_pos[2]))

Output:

.. testoutput:: position

    Start position is [1.54436,0.863271,-1.9297]
    Final position is [1.54.0.85,-1.95]

**Example - Align the Y rotation of bank25 and bank46 in POWGEN:**

.. testcode:: rotation

      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      LoadCalFile(InputWorkspace=ws,
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=True,
	    WorkspaceName="PG3")
      components="bank25,bank46"
      bank25Rot = ws.getInstrument().getComponentByName("bank25").getRotation().getEulerAngles()
      bank46Rot = ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
      print("Start bank25 rotation is [{:.3f}.{:.3f},{:.3f}]".format(bank25Rot[0], bank25Rot[1], bank25Rot[2]))
      print("Start bank46 rotation is [{:.3f}.{:.3f},{:.3f}]".format(bank46Rot[0], bank46Rot[1], bank46Rot[2]))
      AlignComponents(CalibrationTable="PG3_cal",
	      Workspace=ws,
	      MaskWorkspace="PG3_mask",
	      EulerConvention="YZX",
              AlphaRotation=True,
	      ComponentList=components)
      ws=mtd['ws']
      bank25Rot = ws.getInstrument().getComponentByName("bank25").getRotation().getEulerAngles()
      bank46Rot = ws.getInstrument().getComponentByName("bank46").getRotation().getEulerAngles()
      print("Final bank25 rotation is [{:.3f}.{:.3f},{:.3f}]".format(bank25Rot[0], bank25Rot[1], bank25Rot[2]))
      print("Final bank46 rotation is [{:.2f}.{:.3f},{:.3f}]".format(bank46Rot[0], bank46Rot[1], bank46Rot[2]))

Output:

.. testoutput:: rotation

      Start bank25 rotation is [-24.089.0.179,9.030]
      Start bank46 rotation is [-41.092.0.061,17.795]
      Final bank25 rotation is [-24.089.0.179,9.030]
      Final bank46 rotation is [-37.40.0.061,17.795]

**Example - Align sample position in POWGEN:**

.. testcode:: sample

      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2015-08-01.xml")
      LoadCalFile(InputWorkspace=ws,
	    CalFilename="PG3_golden.cal",
	    MakeGroupingWorkspace=False,
	    MakeOffsetsWorkspace=True,
	    MakeMaskWorkspace=True,
	    WorkspaceName="PG3")
      # Mask banks that don't have calibration data
      MaskBTP(Workspace='PG3_mask', Instrument='POWGEN',
	      Bank='22-25,42-45,62-66,82-86,102-105,123,124,143,144,164,184,204')
      print("Start sample position is {}".format(ws.getInstrument().getSample().getPos().getZ()))
      AlignComponents(CalibrationTable="PG3_cal",
            Workspace=ws,
            MaskWorkspace="PG3_mask",
            FitSamplePosition=True,
	    Zposition=True)
      print("Final sample position is {:.3f}".format(mtd['ws'].getInstrument().getSample().getPos().getZ()))

Output:

.. testoutput:: sample

      Start sample position is 0.0
      Final sample position is 0.028

.. categories::

.. sourcelink::
