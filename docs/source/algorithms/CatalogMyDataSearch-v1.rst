.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm obtains the investigations from all active catalogs that exist in *my data* (those of which you are an investigator). If a session is passed to this algorithm then *only* the investigations for that catalog will be returned.

Usage
-----

**Example - obtaining 'My data' from ICAT.**

.. code-block:: python

    # Assuming you have previously logged into the catalog.
    my_data = CatalogMyDataSearch()

    # Verify that we have any investigations in 'My Data'
    print("The number of investigations in 'My data' is: {}".format(len(my_data)))

    # Output the title of each investigation in 'My data'
    for row in my_data:
        print("The title of the investigation is: {}".format(row['Title']))

Output:

.. code-block:: python

    The number of investigations in 'My data' is: 1

    The title of the investigation is: Mantid Test Investigation

.. categories::

.. sourcelink::
