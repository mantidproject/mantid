
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

If a spectrum in the input workspace contains function :math:`f(x)` then the corresponding spectrum in
the output workspace has its indefinite integral:

:math:`\int_{x_0}^x f(\xi)d\xi`.


Usage
-----

**Example - IntegrateFlux**

.. testcode:: IntegrateFluxExample

    # Create an event workspace
    ws = CreateSampleWorkspace("Event")
    # Integrate all spectra.
    wsOut = IntegrateFlux( ws )
    
    # Print the result
    print "The input workspace has %i spectra" % ws.getNumberHistograms()
    print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: IntegrateFluxExample

    The input workspace has 200 spectra
    The output workspace has 200 spectra

.. categories::

