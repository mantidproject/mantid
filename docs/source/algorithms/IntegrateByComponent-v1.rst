.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm integrates up the instrument hierarchy, and each pixel
will contain the average value for the component. For example, assuming
that for a particular instrument on workspace w1 a "tube" is made out of
"pixels", w=IntegrateByComponent(w1,1) will integrate values of w1,
calculate the average along the tube (LevelsUp=1) (for non-masked
pixels), and replace the value of each spectrum in a tube with the
average value for that tube.

Note that if the detectors are grouped before, this algorithm won't run
except with LevelsUp=0 (integrate each spectra individually).


Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: IntegrateByComponent

    #load a workspace with logs
    ws=Load("CNCS_7860")

    #apply algorithm
    ws1=IntegrateByComponent(ws,LevelsUp=1)
    ws2=IntegrateByComponent(ws,LevelsUp=2)
    ws0=IntegrateByComponent(ws,LevelsUp=4)

    #Check some values
    print("For LevelsUp=1 we found that:")
    print(" - two pixels in the same tube have the same value {}".format(ws1.dataY(1)[0]==ws1.dataY(100)[0]))
    print(" - two pixels in different tubes have the same value {}".format(ws1.dataY(1)[0]==ws1.dataY(200)[0]))
    print(" - two pixels in different banks have the same value {}".format(ws1.dataY(1)[0]==ws1.dataY(2000)[0]))

    print("For LevelsUp=2 we found that:")
    print(" - two pixels in the same tube have the same value {}".format(ws2.dataY(1)[0]==ws2.dataY(100)[0]))
    print(" - two pixels in different tubes have the same value {}".format(ws2.dataY(1)[0]==ws2.dataY(200)[0]))
    print(" - two pixels in different banks have the same value {}".format(ws2.dataY(1)[0]==ws2.dataY(2000)[0]))

    print("For LevelsUp=4 we found that:")
    print(" - two pixels in the same tube have the same value {}".format(ws0.dataY(1)[0]==ws0.dataY(100)[0]))
    print(" - two pixels in different tubes have the same value {}".format(ws0.dataY(1)[0]==ws0.dataY(200)[0]))
    print(" - two pixels in different banks have the same value {}".format(ws0.dataY(1)[0]==ws0.dataY(2000)[0]))


.. testcleanup:: IntegrateByComponent

    DeleteWorkspace('ws')

Output:

.. testoutput:: IntegrateByComponent
    
    For LevelsUp=1 we found that:
     - two pixels in the same tube have the same value True
     - two pixels in different tubes have the same value False
     - two pixels in different banks have the same value False
    For LevelsUp=2 we found that:
     - two pixels in the same tube have the same value True
     - two pixels in different tubes have the same value True
     - two pixels in different banks have the same value False
    For LevelsUp=4 we found that:
     - two pixels in the same tube have the same value True
     - two pixels in different tubes have the same value True
     - two pixels in different banks have the same value True

.. categories::

.. sourcelink::
