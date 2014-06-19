.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the geometry information of the detectors in a workspace into a
PAR format ASCII file. The angular positions and linear sizes of the
detectors are calculated using :ref:`algm-FindDetectorsPar`
algorithm.

Tobyfit PAR file is an ASCII file consisting of the header and 5 or 6
text columns. Mantid generates 6-column files. Header contains the
number of the rows in the phx file excluding the header. (number of
detectors). The column has the following information about a detector:

| `` *``
| `` *         1st column      sample-detector distance (secondary flight path)``
| `` *         2nd  "          scattering angle (deg)``
| `` *         3rd  "          azimuthal angle (deg)``
| `` *                         (west bank = 0 deg, north bank = -90 deg etc.)``
| `` *                         (Note the reversed sign convention wrt.  **.phx** files (:ref:`algm-SavePHX` files)
| `` *         4th  "          width (m)``
| `` *         5th  "          height (m)``
| `` *         6th  "          detector ID    -- Mantid specific. ``
| `` *---``

You should expect to find column 6 to be the detector ID in
Mantid-generated par files only.

Usage
-----

**Example - Save PAR file**

.. testcode:: exSavePAR

   # import os funcions to work with folders
   import os
   # create sample workspace
   ws=CreateSampleWorkspace();
      
   file_name = os.path.join(config["defaultsave.directory"], "TestSavePar.par")
    # save it
   SavePAR(ws,Filename=file_name);

   print "target file exists? {0}".format(os.path.exists(file_name));

.. testcleanup:: exSavePAR

   DeleteWorkspace("ws")
   os.remove(file_name)
  
**Output:**

.. testoutput:: exSavePAR

   target file exists? True

.. categories::
