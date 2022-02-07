.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm retrieves the information of the runs associated with an investigation, and saves the results to a workspace. It is similar to :ref:`algm-CatalogGetDataFiles` but it retrieves only one row per run (as opposed to all of the datafiles for a run) and it gets the information from journal files rather than the catalog, so it does not require a catalog login.

This algorithm is used by the search and autoprocessing functionality in the ``ISIS Reflectometry`` interface.

Usage
-----

**Example - obtain run information for all runs in a given ISIS investigation.**

.. code-block:: python

    runs = ISISJournalGetExperimentRuns(Instrument='GEM', InvestigationId = '1390028', Cycle='13_1')

    # Verify that we have any runs in the returned workspace.
    print("The number of runs in this investigation is: {}".format(len(runs)))

    # Output the run number of the runs related to the given investigation.
    for run in runs:
        print("A run with number '{}' exists.".format(run['Run Number']))

Output:

.. code-block:: python

    The number of runs in this investigation is: 2

    A run with number '62838' exists.
    ...
    A run with number '62839' exists.

.. categories::

.. sourcelink::
