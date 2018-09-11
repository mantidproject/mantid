.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm retrieves the information of the datasets associated to an investigation, and saves the results to a workspace.

Usage
-----

**Example - obtain the datasets for a given catalog investigation.**

.. code-block:: python

    # Assumes you have logged in and stored the session information that is
    # returned from CatalogLogin() inside the session variable.
    datasets = CatalogGetDataSets(InvestigationId = '1193002', Session = session.getPropertyValue("Session"))

    # Verify that we have any datafiles in the returned workspace.
    print("The number of datasets for this investigation is: {}".format(len(datasets)))

Output:

.. code-block:: python

    The number of datafiles in this investigation is: 2

.. categories::

.. sourcelink::
