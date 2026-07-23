.. _WorkbenchScriptRepository:

=================
Script Repository
=================

The script repository is a tool for downloading Mantid Scripts and Reduction Algorithms.

- Its a user and scientific community repository for their scripts.
- It allows downloading scripts and keeping them up to date.

.. note::
   Scripts can only be downloaded from the repository. Uploading scripts to, and
   deleting scripts from, the central repository is not available from Mantid.
   To add, modify or remove scripts, submit a pull request on the
   `script repository on GitHub <https://github.com/mantidproject/scriptrepository>`_.

Installation
------------

Installing the Script Repository is simple:

- Under the ``File`` menu select ``Script Repository`` entry and click ``OK``
- Select the installation directory. *Note: it is recommended to use an empty folder to avoid conflicts with other files*

Features
--------

.. image:: /images/Workbench/ScriptRepository/scriptrepository_main.png
   :scale: 50%
   :align: center


The files and directories within the script repository are presented in tabular
form with the location of the installation on disk presented in the top right.
The columns shown are:

- **Path**: The file or directory name. Directories are marked with a ``>`` and
  can be expanded to reveal the contents of subdirectories.

- **Status**: An icon that defines the state of the object:

  - ⤓ indicates the file/directory exists in the repository but has not been
    downloaded locally. Click the icon to download the item.
    Directories also download all child items.

  - ✓ indicates the file/directory has been downloaded to the
    local machine and the contents match those in the repository.

  No icon is shown for items that exist only on your local machine, or that
  have been modified locally.

- **AutoUpdate**: A checkbox appears next to those items that have been downloaded.
  Checking this box causes the file to be updated with contents from the
  repository when Workbench is restarted.

If a file item is highlighted in the table, the contents of the file will be
displayed in the box marked **Description**.

Clicking the **Reload** button refreshes the state of the files both locally and
on the repository itself. The status column will be updated.

Moving the Install Location
---------------------------

If you wish to move the install location of the repository you will need
to update the Mantid configuration to understand the new location.
In the :ref:`IPython console <WorkbenchIPythonConsole>` run the following
commands:

.. code:: python

   config['ScriptLocalRepository'] = 'NEW-PATH-GOES-HERE'
   # persist for future sessions
   config.saveConfig(config.getUserFilename())

Under the Hood
--------------

The interface presented here is simply a facade over a `Git <https://git-scm.com/>`__
repository stored at https://github.com/mantidproject/scriptrepository.
The full change history can be seen here and any normal git operations
will work with this repository.
