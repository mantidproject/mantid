.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm connects with hte cloud based Mantid Instrument Definition 
Repository and downloads any updated instrument files for use within Mantid.

This algorithm is normally run on startup of Mantid for official installed 
releases of Mantid from version 3.3 onwards.

Mantid Property Keys
####################

The code is configured using he following Mantid.Property, or Mantid.User.Property keys.

UpdateInstrumentDefinitions.OnStartup = 1
   This controls wether this algorithm is run on startup of Mantid.  1 = Run on startup, 0 = don't.

UpdateInstrumentDefinitions.URL = https://api.github.com/repos/mantidproject/mantid/contents/Code/Mantid/instrument
   This stores the url used to access the contents of the Mantid Instrument Definition Repository.

Output Messages
###############

The following may be logged at Notice

All instrument definitions up to date
   Everything is fine, there are now updates to download.

Downloading n files from the instrument repository
   Some files have changed or been added to the Instrument Repository, they are being downloaded.

Internet Connection Failed - cannot update instrument definitions.
   The connection to the instrument Repository failed.  This could be down to your network connection, proxy settings (we use the system proxy settings).  More details will be logged as another message at information level.

Instrument Definition Update: The Github API rate limit has been reached, try again after 19-Feb-2015 11:23:34
   There is a limit of how many calls we are allowed to make to Github per hour.  Try again after the time specified.  If this keeps occuring let the developement team know.

The details
###########

The Mantid Instrument Definition Repository is currently a directory within the Mantid code repository on Github. 
https://github.com/mantidproject/mantid

The instrument files within Mantid can be accessed in three locations.

1. The mantid install directory [Install Dir] (windows default "C:\mantidinstall\instrument").
2. The local user appdata directory [Appdata Dir] (windows "%APPDATA%\mantidproject\mantid\instrument", linux/Mac "~/.mantid/instrument").
3. Mantid Instrument repository [Instrument Repo] on Github

When running the algorithm the processing takes the following steps.

1. The description of the contents of the Instrument Repo is downloaded and stored in a file github.json to the Appdata Dir.
2. A file of the desription of xml files in  Appdata Dir and Install Dir are updated or created and saved into AppData Dir as install.json and appdata.json.
3. The contents of all 3 are inspected and new or updated files (based on the git checksum) are added to a download list.
4. The list of files are downloaded to the Appdata Dir.
5. :ref:`algm-LoadInstrument` will load files in Appdata Dir in preference to those in the Install Dir if both are valid.

Usage
-----

**Example**

.. testcode:: Ex

   updatedFileCount = DownloadInstrument()
   print("The number of files updated was " + str(updatedFileCount))

Output:

.. testoutput:: Ex
   :options: +ELLIPSIS

   The number of files updated was ...

.. categories::
