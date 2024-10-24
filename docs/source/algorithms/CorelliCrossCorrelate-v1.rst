
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm calculates the elastic signal for the Corelli diffractometer. This is done by calculating the cross-correlation with the correlation chopper. The correlation chopper modulates the incident neutron beam with a pseudo-random sequence. The calculated signal is applied the each event in the form of a weight.

The weights that the events are scaled by are :math:`1` for transparent and :math:`\frac{-c}{1-c}` for absorbing, where :math:`c` is the duty cycle (:math:`c\approx0.498475`).

The algorithm requires the timing offset of the TDC signal from the correlation chopper to run. The timing offset is dependent on the frequency of the chopper and should not change if the frequency has not changed.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CorelliCrossCorrelate**

.. testcode:: CorelliCrossCorrelateExample

    # Load a Corelli data file.
    ws = Load('CORELLI_2100')

    # You will need to load the instrument if the one in the NeXus file doesn't contain the chopper sequence.
    LoadInstrument(ws, InstrumentName='CORELLI', RewriteSpectraMap=False)

    # Run the cross-correlation. This is using a TDC timing offset of 56000ns.
    wsOut = CorelliCrossCorrelate(ws, 56000)

    print('The detector 172305 has {} events.'.format(ws.getSpectrum(172305).getNumberEvents()))
    print('The event weights before cross-correlation are {}'.format(ws.getSpectrum(172305).getWeights()))
    print('The event weights after  cross-correlation are {}'.format(wsOut.getSpectrum(172305).getWeights()))

Output:

.. testoutput:: CorelliCrossCorrelateExample

    The detector 172305 has 3 events.
    The event weights before cross-correlation are [1. 1. 1.]
    The event weights after  cross-correlation are [-0.99391854  1.          1.        ]

.. categories::

.. sourcelink::
