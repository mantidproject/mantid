.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is then used to mark all 'dead' detectors with a 'dead' marker
value, while all spectra from live detectors are given a 'live' marker
value.

This algorithm is primarily used to ease identification using the
instrument visualization tools.

ChildAlgorithms used
####################

Uses the :ref:`algm-Integration` algorithm to sum the spectra.


Usage
-----

**Example - Find various dead detectors**  

.. testcode:: FindVariousDeadDets

    import numpy as np

    ws = CreateSampleWorkspace(BinWidth=2000)
    #set some detectors as dead
    #First - very dead all bins have zero counts
    vd_data=[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]
    ws.setY(10,np.array(vd_data))
    #second - drop off some counts, then drops to 0
    do_data=[2.0,5.0,4.0,1.0,0.0,0.0,0.0,0.0,0.0,0.0]
    ws.setY(15,np.array(do_data))
    #third strange some counts, then drops to 0, then recovers
    str_data=[0.4,5.0,0.001,0.0,0.0,0.0,0.0,0.0,1.0,1.0]
    ws.setY(20,np.array(str_data))

    print("With no range will find very dead")
    (wsOut,detList) = FindDeadDetectors(ws)
    print(detList)

    print("\nwith a lower range will find very dead and drop off")
    (wsOut,detList) = FindDeadDetectors(ws,RangeLower=8e3)
    print(detList)

    print("\nwith a lower range and upper range will find all three")
    (wsOut,detList) = FindDeadDetectors(ws,RangeLower=8e3, rangeUpper=1.6e4)
    print(detList)

Output:

.. testoutput:: FindVariousDeadDets

    With no range will find very dead
    [110]

    with a lower range will find very dead and drop off
    [110,115]

    with a lower range and upper range will find all three
    [110,115,120]
 



.. categories::

.. sourcelink::
