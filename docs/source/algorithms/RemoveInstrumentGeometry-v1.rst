
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Remove Instrument Geometry from workspace.


Usage
-----

**Example - RemoveInstrumentGeometry**

.. testcode:: RemoveInstrumentGeometryExample

    # create some workspace with an instrument
    ws = CreateSampleWorkspace()
    print("Instrument Geometry exists:  {}".format(ws.getInstrument().nelements() > 0))

    # delete instrument geometry
    ws = RemoveInstrumentGeometry(ws)
    print("Instrument Geometry is empty:  {}".format(ws.getInstrument().nelements() == 0))


.. testcleanup:: RemoveInstrumentGeometryExample

   DeleteWorkspace(ws)



Output:

.. testoutput:: RemoveInstrumentGeometryExample

    Instrument Geometry exists:  True
    Instrument Geometry is empty:  True

.. categories::

.. sourcelink::
