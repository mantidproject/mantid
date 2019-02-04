.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm retrieves the information of the datafiles associated to an investigation, and saves the results to a workspace.

Usage
-----

**Example - obtain datafile information for all datafiles in a given ICAT investigation.**

.. code-block:: python

    # Assumes you have logged in and stored the session information that is
    # returned from CatalogLogin() inside the session variable.
    datafiles = CatalogGetDataFiles(InvestigationId = '1390028', Session = session.getPropertyValue("Session"))

    # Verify that we have any datafiles in the returned workspace.
    print("The number of datafiles in this investigation is: {}".format(len(datafiles)))

    # Output the ID of the datafiles related to the given investigation.
    for row in datafiles:
        print("A datafile with id '{}' exists.".format(row['Id']))

Output:

.. code-block:: python

    The number of datafiles in this investigation is: 22

    A datafile with id '33121358' exists.
    ...
    A datafile with id '33121573' exists.

.. categories::

.. sourcelink::
