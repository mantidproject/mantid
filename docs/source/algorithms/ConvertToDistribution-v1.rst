.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Makes a histogram workspace a distribution i.e. divides by the bin width.

Restrictions on the input workspace
###################################

The workspace to convert must contain histogram data which is not already
flagged as a distribution.
It cannot be an :ref:`EventWorkspace <EventWorkspace>`.

Usage
-----

**Example - converting multiple peaks to distribution**

.. testcode:: ConvertToDistribution

    ws_multi = CreateSampleWorkspace("Histogram", "Multiple Peaks")

    print("Is the workspace a distribution? {}".format(ws_multi.isDistribution()))
    print("The workspace has a level background of {} counts.".format(ws_multi.readY(0)[0]))
    print("The largest of which is {} counts.\n".format(ws_multi.readY(0)[60]))

    ConvertToDistribution(ws_multi)

    print("Is the workspace a distribution? {}".format(ws_multi.isDistribution()))
    print("The workspace has a level background of {} counts.".format(ws_multi.readY(0)[0]))
    print("The largest of which is {} counts.".format(ws_multi.readY(0)[60]))

Output:

.. testoutput:: ConvertToDistribution

    Is the workspace a distribution? False
    The workspace has a level background of 0.3 counts.
    The largest of which is 8.3 counts.

    Is the workspace a distribution? True
    The workspace has a level background of 0.0015 counts.
    The largest of which is 0.0415 counts.

.. categories::

.. sourcelink::
