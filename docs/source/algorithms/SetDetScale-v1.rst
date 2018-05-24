.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Given a PeaksWorkspace or MatrixWorkspace with an instrument, this
algorithm will set or change the detector bank scales that are used in
SaveHKL and AnvredCorrection.  The input format is the same as
used in anvred3.py, so DetScaleList input can be pasted from
the definition of detScale there.  Alternately, there is an option, DetScaleFile, to
read a text file with each line containing the detector number and scale factor for that detector.  
If scales for a detector are given in both the DetScaleList text string and DetScaleFile,
the values from the text string will be used.  The default values can be 
set in the instrument parameter file.

Usage
-----

**Example:**

.. testcode:: ExSetDetScale

    w=LoadIsawPeaks("TOPAZ_3007.peaks")

    print("Before SetDetScale:")
    print('{0:.5f}'.format(w.getInstrument().getNumberParameter("detScale17")[0]))
    print('{0:.5f}'.format(w.getInstrument().getNumberParameter("detScale49")[0]))

    #This SetDetScale will change the parameters set in parameter file
    SetDetScale(Workspace=w, DetScaleList='17:1.0,49:2.0')
    print("After SetDetScale:")
    print('{0:.5f}'.format(w.getInstrument().getNumberParameter("detScale17")[0]))
    print('{0:.5f}'.format(w.getInstrument().getNumberParameter("detScale49")[0]))


Output:

.. testoutput:: ExSetDetScale

    Before SetDetScale:
    1.18898
    0.79420
    After SetDetScale:
    1.00000
    2.00000


.. categories::

.. sourcelink::
