.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates an XML Grouping file of the form:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8" ?>
    <detector-grouping>
    <group name="fwd"><detids val="1,2,17,32"/></group>
    <group name="bwd"><detids val="33,36,38,60,64"/> </group>
    </detector-grouping>

Based on information retrieved from the
`Nearest Neighbours <http://www.mantidproject.org/Nearest_Neighbours>`_
class in Mantid Geometry.

Usage
-----

.. caution::

    The following is a contrived example to show the functionaity of the algorithm,
    However, since the sample workspace does not have a valid instrument, the written
    grouping file is invalid and cannot be used.

.. testcode:: Ex

    # Create a workspace with a single bank and 9x9 pixels.
    ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=9)
    import os
    group_file = os.path.join(config["default.savedirectory"], "mygroup.xml")
    # Want to get nine groups of 3x3
    SpatialGrouping(ws, group_file, SearchDistance=10)
    print("File created: {}".format(os.path.exists(group_file)))

Output:

.. testoutput:: Ex

    File created: True

.. testcleanup:: Ex

    import os
    os.remove(group_file)

.. categories::

.. sourcelink::
