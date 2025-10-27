.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A utility algorithm for grouping detector components together based on repeated component names
(i.e. if you wanted to group all the pixels in each module together). This is best illustrated by example.

You have an instrument that has the following hierarchy Banks -> Module -> Pixels.
Let's say there are two banks called bank1 and bank2. Then each bank is made up of three module named module1, module2 and module3, respectively.
Finally each of these module contains 5 detector pixels: pixel1, pixel2, pixel3, pixel4, and pixel5

If you want to create a GroupingWorkspace where each module forms its own detector group, you formulate this as grouping together "pixel" components
belonging to the same parent component (in this case all the "pixel" components under each module). Likewise, to get a ``GroupingWorkspace`` where each
bank is independent you would use ``IncludeComponents = "module"``, as this would group together each set of three modules in the two banks.

In some situations you may have undesirable components in the instrument tree such as ``"transmission-pixel1"``
which exactly contain the name of the actual component you are looking to match. In these cases you can use ``ExcludeComponents`` to remove these from groupings.
Additionally, entire branches of the Instrument Component Tree can be excluded with ``ExcludeBranches`` no components will be grouped under any parent component
with names containing any of the comma separated strings provided here.

Usage
-----

**Example: Grouping ENGINX pixels together, excluding the transmission pixels**

.. testcode:: CreateGroupingByComponent

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX", OutputWorkspace = "test_group", IncludeComponents = "pixel",  ExcludeComponents = "transmission", GroupSubdivision = 1)

**Example: Grouping ENGINX pixels together, excluding the NorthBank and TransmissionBank**

.. testcode:: CreateGroupingByComponent

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX",
                              OutputWorkspace = "test_group",
                              IncludeComponents = "pixel",
                              ExcludeBranches = "NorthBank, Transmission",
                              GroupSubdivision = 1)
    # note that either the exact string "NorthBank" or substring "Transmission" can be provided as comma separated strings


.. categories::

.. sourcelink::
