.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Given a PeaksWorkspace or MatrixWorkspace with an instrument, this
algorithm will set or change the detector bank scales that are used in
SaveHKL and AnvredCorrection.  The input format is the same as
used in anvred3.py, so DetScaleList input can be pasted from
the definition of detScale there.  The default values can be 
set in the instrument parameter file.

Usage
-----

**Example:**

.. testcode:: ExSetDetScale

    w=LoadIsawPeaks("TOPAZ_3007.peaks")

    print "Before SetDetScale:"
    print w.getInstrument().getNumberParameter("detScale17")[0]
    print w.getInstrument().getNumberParameter("detScale49")[0]

    #This SetDetScale will change the parameters set in parameter file
    SetDetScale(Workspace=w, DetScaleList='17:1.0,49:2.0')
    print "After SetDetScale:"
    print w.getInstrument().getNumberParameter("detScale17")[0]
    print w.getInstrument().getNumberParameter("detScale49")[0]


Output:

.. testoutput:: ExSetDetScale

    Before SetDetScale:
    1.18898
    0.79420
    After SetDetScale:
    1.0
    2.0


.. categories::

.. sourcelink::
