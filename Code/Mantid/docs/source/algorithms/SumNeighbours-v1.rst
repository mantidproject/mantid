.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm looks through the :ref:`Instrument <Instrument>` to find all
the `RectangularDetectors <http://www.mantidproject.org/RectangularDetector>`__ defined. For each
detector, the SumX\*SumY neighboring event lists are summed together and
saved in the output workspace as a single spectrum. Therefore, the
output workspace will have 1/(SumX\*SumY) \* the original number of
spectra.


Usage
-----

**Example:**

.. testcode:: ExSumNeighbours

    ws = CreateSampleWorkspace(BankPixelWidth=10,NumBanks=2)
    print "ws has %i spectra to begin with" % ws.getNumberHistograms()
    wsOut = SumNeighbours(ws,5,5)

    print "wsOut has only %i spectra after beimng summed into 5x5 blocks" % wsOut.getNumberHistograms()

Output:

.. testoutput:: ExSumNeighbours

    ws has 200 spectra to begin with
    wsOut has only 8 spectra after beimng summed into 5x5 blocks


.. categories::
