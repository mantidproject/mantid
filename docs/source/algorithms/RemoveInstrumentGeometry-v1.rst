
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
    print("Instrument Geometry exists:  {}".format(!ws.getInstrument().isEmptyInstrument()))

    # delete instrument geometry
    RemoveInstrumentGeometry(ws)
    print("Instrument Geometry exists (should be false):  {}".format(!ws.getInstrument().isEmptyInstrument()))

.. testcleanup:: RemoveInstrumentGeometry

   DeleteWorkspace(ws)



Output:

.. testoutput:: RemoveInstrumentGeometryExample

    Instrument Geometry exists:  True
    Instrument Geometry exists (should be false):  False

.. categories::

.. sourcelink::

