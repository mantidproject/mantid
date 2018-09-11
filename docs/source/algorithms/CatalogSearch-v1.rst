.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm searches through the all active catalogs for investigations based on the parameters set, and saves the results into a workspace.

Usage
-----

**Example - searches for investigations in the catalog based on parameters set.**

.. code-block:: python

    # When logging into the catalog the session class is returned.
    # This can then be used throughout to perform other ICAT routines.
    search_results = CatalogSearch(Instrument="ALF",limit=3)

    # A tuple is returned from CatalogSearch(). The first item is the search results.
    investigations = search_results[0]

    print("The number of investigations returned was: {}".format(len(investigations)))

    # Print the title of all returned investigations.
    for investigation in investigations:
	    print("The title of the investigation is: {}".format(investigation['Title']))

    # Log out of the catalog, otherwise results are appended to the workspace (search_results).
    CatalogLogout()

Output:

.. code-block:: python

    The number of investigations returned was: 3

    The title of the investigation is: CAL_ALF_14/02/2014 15:37:05
    The title of the investigation is: CAL_ALF_18/11/2013 15:03:48
    The title of the investigation is: CAL_ALF_19/11/2013 11:59:56

**Example - paging results returned.**

.. code-block:: python

    # When CountOnly is set to True a separate COUNT query is performed to see the total
    # number of investigations that will be returned by this search. This is used for paging functionality.
    # If CountOnly is not provided, then ALL results are returned. This can be extremely slow.
    search_results = CatalogSearch(Instrument="ALF",CountOnly=True,StartDate="03/06/2012", EndDate="03/06/2014")

    print("The number of search results returned was: {}".format(search_results[1]))

    CatalogLogout()

Output:

.. code-block:: python

     The number of search results returned was: 109

.. categories::

.. sourcelink::
     :h: Framework/ICat/inc/MantidICat/CatalogSearch.h
     :cpp: Framework/ICat/src/CatalogSearch.cpp
