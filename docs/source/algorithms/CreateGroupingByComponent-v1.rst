.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

A utility algorithm for grouping detector components together based on repeated component names
(i.e. if you wanted to group all the pixels in each module together). This is best illustrated by example.

You have an instrument that has the following hierarchy Banks -> Module -> Pixels.
Let's say there are two banks called ``bank1`` and ``bank2``. Then each bank is made up of three module named
``module1``, ``module2`` and ``module3``, respectively.
Finally each of these module contains 5 detector pixels: ``pixel1``, ``pixel2``, ``pixel3``, ``pixel4``, and ``pixel5``

If you want to create a ``GroupingWorkspace`` where each module forms its own detector group, you formulate this as grouping together "pixel" components
belonging to the same parent component, with  ``ComponentNameIncludes = "pixel"`` (in this case all the "pixel" components under each module).
Likewise, to get a ``GroupingWorkspace`` where each
bank is independent you would use ``ComponentNameIncludes = "module"``, as this would group together each set of three modules in the two banks.

In some situations you may have undesirable components in the instrument tree which exactly contain the name of the
actual component you are looking to match (for example ``transmission-pixel1`` when searching for ``pixel`` components).
In these cases you can use ``ComponentNameExcludes`` to remove these from groupings.
Additionally, entire branches of the Instrument Component Tree can be excluded with ``ExcludeBranches``,
no components will be grouped under any parent component
with names containing any of the comma separated strings provided here.

Usage
-----

**Example: Grouping ENGINX blocks together**

.. testcode:: CreateGroupingByComponent-example

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX",
                              OutputWorkspace = "test_group",
                              ComponentNameIncludes = "block",
                              ComponentNameExcludes = "transmission",
                              GroupSubdivision = 1)


.. figure:: /images/CreateGroupingByComponent-ENGINXmodule.png
   :alt: CreateGroupingByComponent-ENGINXmodule.png
   :align: center
   :width: 600 px


**Example: Grouping ENGINX blocks together, then subdividing by 3**

.. testcode:: CreateGroupingByComponent-exampleSubdivision

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX",
                              OutputWorkspace = "test_group",
                              ComponentNameIncludes = "block",
                              ComponentNameExcludes = "transmission",
                              GroupSubdivision = 3)


.. figure:: /images/CreateGroupingByComponent-ENGINXmoduleSub3.png
   :alt: CreateGroupingByComponent-ENGINXmoduleSub3.png
   :align: center
   :width: 600 px

**Example: Grouping ENGINX pixels together, excluding transmission-pixels**

.. testcode:: CreateGroupingByComponent-excludeString

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX",
                              OutputWorkspace = "test_group",
                              ComponentNameIncludes = "pixel",
                              ComponentNameExcludes = "transmission",
                              GroupSubdivision = 1)


.. figure:: /images/CreateGroupingByComponent-ENGINXpixel.png
   :alt: CreateGroupingByComponent-ENGINXpixel.png
   :align: center
   :width: 600 px


**Example: Grouping ENGINX pixels together, excluding the NorthBank and TransmissionBank**

.. testcode:: CreateGroupingByComponent-excludeBranch

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    CreateGroupingByComponent(InstrumentName = "ENGINX",
                              OutputWorkspace = "test_group",
                              ComponentNameIncludes = "pixel",
                              ExcludeBranches = "NorthBank, Transmission",
                              GroupSubdivision = 1)
    # note that either the exact string "NorthBank" or substring "Transmission" can be provided as comma separated strings


.. figure:: /images/CreateGroupingByComponent-ENGINXpixelExclude.png
   :alt: CreateGroupingByComponent-ENGINXpixelExclude.png
   :align: center
   :width: 600 px


.. categories::

.. sourcelink::
