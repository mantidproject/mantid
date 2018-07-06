.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Converts a histogram workspace from a distribution i.e. multiplies by the bin width to take out the bin width dependency.
This algorithm is often used with :ref:`ConvertToDistribution <algm-ConvertToDistribution-v1>`.

Restrictions on the input workspace
###################################

The workspace to convert must contain histogram data which is flagged as being a distribution.

Usage
-----

**Example - converting a workspace to and from a distribution**

.. testcode:: ConvertFromDistribution

    # By default it is not a distribution.
    ws_multi = CreateSampleWorkspace("Histogram", "Multiple Peaks")

    # Convert to a distribution for demonstration purposes.
    # If your data is already distributed then this is not required.
    ConvertToDistribution(ws_multi)

    print("Is the workspace a distribution? {}".format(ws_multi.isDistribution()))

    # Convert back to the initial workspace state.
    ConvertFromDistribution(ws_multi)

    print("Is the workspace a distribution? {}".format(ws_multi.isDistribution()))

Output:

.. testoutput:: ConvertFromDistribution

    Is the workspace a distribution? True
    Is the workspace a distribution? False

.. categories::

.. sourcelink::
