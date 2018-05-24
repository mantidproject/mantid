.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm applies a :ref:`calibration table
<DiffractionCalibrationWorkspace>` to convert a workspace from
time-of-flight to dSpacing as described below. The equation in `GSAS
<https://subversion.xor.aps.anl.gov/trac/pyGSAS>`_ converts from
d-spacing (:math:`d`) to time-of-flight (:math:`TOF`) by the equation:

.. math:: TOF = DIFC * d + DIFA * d^2 + TZERO

The manual describes these terms in more detail. Roughly,
:math:`TZERO` is related to the difference between the measured and
actual time-of-flight base on emission time from the moderator, :math:`DIFA` is an empirical term (ideally zero), and :math:`DIFC` is

.. math:: DIFC = \frac{2m_N}{h} L_{tot} sin \theta

Measuring peak positions using a crystal with a very well known
lattice constant will give a good value for converting the data. The
d-spacing of the data will be calculated using whichever equation
below is appropriate for solving the quadratic.

When :math:`DIFA = 0` then the solution is just for a line and

.. math:: d = \frac{TOF - TZERO}{DIFC}

For the case of needing to solve the actual quadratic equation

.. math:: d = \frac{-DIFC}{2 DIFA} \pm \sqrt{\frac{TOF}{DIFA} + \left(\frac{DIFC}{2 DIFA}\right)^2 - \frac{TZERO}{DIFA}}

Here the positive root is used when :math:`DIFA > 0` and the negative
when :math:`DIFA < 0`.

This algorithm always uses a :ref:`calibration table
<DiffractionCalibrationWorkspace>` which it either reads from the
`CalibrationWorkspace` property, or uses :ref:`ConvertDiffCal
<algm-ConvertDiffCal>` or :ref:`LoadCalFile <algm-LoadCalFile>` to
produce.

**Note:** the workspace that this algorithms outputs is a 
:ref:`ragged workspace <Ragged_Workspace>`.

Restrictions on the input workspace
###################################

The input workspace must contain histogram or event data where the X
unit is time-of-flight and the Y data is raw counts. The
:ref:`instrument <instrument>` associated with the workspace must be
fully defined because detector, source & sample position are needed if
an OffsetsWorkspace is provided.

Usage
-----

**Example: Use offset to move peak in Dspace**

.. testcode:: ExAlignDetectors

    ws = CreateSampleWorkspace("Event",NumBanks=1,BankPixelWidth=1)
    ws = MoveInstrumentComponent(Workspace='ws', ComponentName='bank1', X=0.5, RelativePosition=False)
    wsD = ConvertUnits(InputWorkspace='ws',  Target='dSpacing')
    maxD = Max(wsD)
    offset = GetDetectorOffsets(InputWorkspace='wsD', DReference=2.5, XMin=2, XMax=3)
    wsA = AlignDetectors(InputWorkspace='ws', OutputWorkspace='wsA', OffsetsWorkspace='offset')
    maxA = Max(wsA)
    print("Peak in dSpace {:.11f}".format(maxD.readX(0)[0]))
    print("Peak from calibration {:.10f}".format(maxA.readX(0)[0]))

Output:

.. testoutput:: ExAlignDetectors

    Peak in dSpace 2.66413186052
    Peak from calibration 2.5596132087


.. categories::

.. sourcelink::
