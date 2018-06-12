.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Performs ki / kf multiplication, in order to transform differential
scattering cross section into dynamic structure factor. Both Ei and Ef
must be positive. However, if this requirement is not met, it will give
an error only if the data is not 0. This allows applying the algorithms
to rebinned data, where one can rebin in Direct EMode to energies higher
than EFixed. If no value is defined for EFixed, the algorithm will try
to find Ei in the workspace properties for direct geometry spectrometry,
or in the instrument definition, for indirect geometry spectrometry.
Algorithm is event aware. TOF events will be changed to weighted events.


Usage
-----

**Example**  

.. testcode:: CorrectKiKf

    ws = CreateSampleWorkspace()
    ws = ConvertUnits(ws,"DeltaE",EMode="Direct", EFixed=7.5)
    
    wsOut = CorrectKiKf(ws, EMode="Direct", EFixed=7.5)

    print("First five bins:")
    print("index  orig  corrected")
    for i in range(5):
        print("  %i    %.2f  %.2f" % 
            (i,ws.readY(0)[i],wsOut.readY(0)[i]))


Output:

.. testoutput:: CorrectKiKf

    First five bins:
    index  orig  corrected
      0    0.30  0.01
      1    0.30  0.02
      2    0.30  0.04
      3    0.30  0.05
      4    0.30  0.07


.. categories::

.. sourcelink::
