.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm obtains a list of investigation types for all catalogs by default or a specific session if specified.

Usage
-----

**Example - obtain the investigation types from current catalogs.**

.. code-block:: python

    # Assuming you have previously logged into the ISIS catalog.
    investigation_types = CatalogListInvestigations()

    # How many different types of investigations are at ISIS?
    print("The number of investigation types are: {}".format(len(investigation_types)))

    # Print and view the investigation types
    for investigation in investigation_types:
        print("Investigation type is: {}".format(investigation))

Output:

.. code-block:: python

    The number of instruments at ISIS is: 9

    Investigation type is: Disordered Materials Published Data
    ...
    Investigation type is: unknown

.. categories::

.. sourcelink::
