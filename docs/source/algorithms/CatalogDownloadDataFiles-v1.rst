.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm downloads a datafile or multiple datafiles based on the given ID of a datafile in an investigation. It first checks if the file can be downloaded from the data archives (based on information in the config file Facilities.xml) and downloads the file from there. Otherwise, the file is downloaded over HTTPS.

The datafile ID to use to download a datafile can be obtained from :ref:`CatalogGetDataFiles <algm-CatalogGetDataFiles>`.

Usage
-----

**Example - downloading a single datafile from ICAT.**

.. code-block:: python

    # Assuming you have previously logged into the catalog
    # and stored the session return value in the variable 'session'
    CatalogDownloadDataFiles(FileIds = '33127010', # Which file to find?
                             FileNames = '33127010.nxs', # What to name it?
                             DownloadPath = '~/Desktop',  # Where to save it?
                             Session = session.getPropertyValue("Session") # Which session to use?
                            )

Output:

.. code-block:: python

    # Outputs a list locations of the datafiles downloaded.
    # USERNAME will be your username on your system.
    ['/home/USERNAME/Desktop/33127010.nxs']

**Example - downloading multiple datafiles from ICAT.**

.. code-block:: python

    # The ids of the datafiles to download.
    # These could be obtained from CatalogGetDataFiles or entered manually.
    datafile_ids = [33127010, 33127011]

    # This option will not append any file extensions to the datafiles.
    CatalogDownloadDataFiles(FileIds = datafile_ids,    # We now pass our list of ids here
                             # The filenames are those that will be saved on your machine
                             # I have opted to use the datafile ids for this, but it is possible
                             # to provide a list of desired names.
                             # This must be the same size as the FileIds list.
                             FileNames = datafile_ids,
                             DownloadPath = '~/Desktop',
                             Session = session.getPropertyValue("Session")
                             )

Output:

.. code-block:: python

    ['/home/USERNAME/Desktop/33127010.nxs', '/home/USERNAME/Desktop/33127011.nxs']

.. categories::

.. sourcelink::
