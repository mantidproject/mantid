
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will take the output of :ref:`algm-GetDetOffsetsMultiPeaks` in the form of a
:ref:`diffractioncalibration workspace <DiffractionCalibrationWorkspace>` and minimize the difference between the DIFC
of the instrument and calibration workspace by moving and rotating instrument components.

Usage
-----

.. testcode:: DoIt

      LoadCalFile(InstrumentName="PG3",
            CalFilename="PG3_golden.cal",
            MakeGroupingWorkspace=False,
            MakeOffsetsWorkspace=True,
            MakeMaskWorkspace=False,
            WorkspaceName="PG3")
      ws = LoadEmptyInstrument(Filename="POWGEN_Definition_2014-03-10.xml")
      startPos = ws.getInstrument().getComponentByName("bank63").getPos()
      startRot = ws.getInstrument().getComponentByName("bank63").getRotation()
      AlignComponents(CalibrationTable="PG3_cal",
                      InputWorkspace=ws,
                      ComponentList="bank63")
      endPos = ws.getInstrument().getComponentByName("bank63").getPos()
      endRot = ws.getInstrument().getComponentByName("bank63").getRotation()

Output:

.. testoutput:: DoIt

    File created: True

.. categories::

.. sourcelink::
