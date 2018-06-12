.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm allows a user (who is logged into a catalog) to
publish datafiles or workspaces to investigations of which they
are an investigator.Datafiles and workspaces that are published
are automatically made private.
This means only investigators of that investigation can view them.

**Note:** the catalog publish dialog is disabled if you are not an investigator on any investigations. This prevents attempts to publish to investigations of which you do not have access, which would result in an error.

Parameters Note
###############

-  A file or workspace can be published, but not both at the same time.
-  When uploading a workspace, it is saved to the default save directory
   as a nexus file. The history of of the workspace is generated and
   saved into a Python script, which is also uploaded alongside the
   datafile of the workspace.

Usage
-----

**Example - publish a datafile directly to the archives.**

**CAUTION:** This usage example will only work if you are an investigator on the given investigation.

.. code-block:: python

    # To ensure the datafile is published to the correct catalog,
    # store the session returned when logging in for use later.
    session = CatalogLogin(USERNAME,PASSWORD)

    # Publish a datafile to the archives.
    CatalogPublish(
        # The location of the datafile to publish to the archives.
        FileName="~Desktop/exampleFile.nxs",
        # The investigation to publish the datafile to. This could be obtained
        # dynamically from CatalogSearch().
        InvestigationNumber = 1193002,
        # The description to save to the datafile in the archives.
        DataFileDescription = "This datafile demonstrates the use of workflow N",
        # Used to ensure the file is published to the correct catalog.
        # This is required if you are logged into more than one catalog.
        Session = session.getPropertyValue("Session")
    )

.. categories::

.. sourcelink::
