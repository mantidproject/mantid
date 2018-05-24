.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the geometry information of the detectors in a workspace into a
PHX format ASCII file. The angular positions and angular sizes of the
detectors are calculated using :ref:`algm-FindDetectorsPar`
algorithm.

Mantid generated PHX file is an ASCII file consisting of the header and
7 text columns. Header contains the number of the rows in the phx file
excluding the header (number of detectors). The column has the
following information about a detector:


+---------------+-------------------------------------------------------------------------------------------------------+
| Column Number |                                Column Description                                                     |
+===============+=======================================================================================================+
|  1st          |  secondary flightpath,e.g. sample to detector distance (m)                                            |
+---------------+-------------------------------------------------------------------------------------------------------+
|  2nd          |  0                                                                                                    |
+---------------+-------------------------------------------------------------------------------------------------------+
|  3rd          |  scattering angle (deg)                                                                               |
+---------------+-------------------------------------------------------------------------------------------------------+
|  4th          |  azimuthal angle (deg) (west bank = 0 deg, north bank = 90 deg etc.)                                  |
|               |  Note the reversed sign convention wrt the **.par** files. For details, see: :ref:`algm-SavePAR`      |
+---------------+-------------------------------------------------------------------------------------------------------+
|  5th          |  angular width e.g. delta scattered angle (deg)                                                       |
+---------------+-------------------------------------------------------------------------------------------------------+
|  6th          |  angular height e.g. delta azimuthal angle (deg)                                                      |
+---------------+-------------------------------------------------------------------------------------------------------+
|  7th          |  detector ID   -- this is Mantid specific value, which may not                                        |
|               | hold similar meaning in files written by different applications.                                      |
+---------------+-------------------------------------------------------------------------------------------------------+


In standard **phx** file only the columns 3,4,5 and 6 contain useful
information. You can expect to find column 1 to be the secondary
flightpath and the column 7 -- the detector ID in Mantid-generated phx
files only.

Usage
-----

**Example - Save PHX file**

.. testcode:: exSavePHX

   # import os funcions to work with folders
   import os
   # create sample workspace
   ws=CreateSampleWorkspace()
   # test file name
   file_name = os.path.join(config["defaultsave.directory"], "TestSavePhx.phx")
    # save the file
   SavePHX(ws,Filename=file_name);

   print("target file exists? {0}".format(os.path.exists(file_name)))

.. testcleanup:: exSavePHX

   DeleteWorkspace("ws")
   os.remove(file_name)
  
**Output:**

.. testoutput:: exSavePHX

   target file exists? True


.. categories::

.. sourcelink::
