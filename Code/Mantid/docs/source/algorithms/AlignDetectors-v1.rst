.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The offsets are a correction to the dSpacing values and are applied
during the conversion from time-of-flight to dSpacing as follows:

.. math:: d = \frac{h}{2m_N} \frac{t.o.f.}{L_{tot} sin \theta} (1+ \rm{offset})

The detector offsets can be obtained from either: an OffsetsWorkspace where each pixel has one value,
the offset; or a .cal file (in the form created by the ARIEL software).

**Note:** the workspace that this algorithms outputs is a `Ragged
Workspace <http://www.mantidproject.org/Ragged_Workspace>`__.

Restrictions on the input workspace
###################################

The input workspace must contain histogram or event data where the X
unit is time-of-flight and the Y data is raw counts. The
:ref:`instrument <instrument>` associated with the workspace must be fully
defined because detector, source & sample position are needed.

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
    print "Peak in dSpace", maxD.readX(0)[0]
    print "Peak from calibration", maxA.readX(0)[0]

Output:

.. testoutput:: ExAlignDetectors

    Peak in dSpace 2.66413186052
    Peak from calibration 2.56009958218


.. categories::
