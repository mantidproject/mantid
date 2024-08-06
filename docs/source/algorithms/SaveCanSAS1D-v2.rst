.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. _version 1.0: http://www.cansas.org/formats/1.0/cansas1d.xsd
.. _Version 1.1: http://www.cansas.org/formats/1.1/cansas1d.xsd
.. _canSAS Wiki: http://www.cansas.org/formats/canSAS1d/1.1/doc/

Description
-----------

Saves the given :ref:`MatrixWorkspace` to a file in the CanSAS1D XML format.

This format is only intended to be used for 1D workspaces.

If the workspace contains several spectra, two options are available:

* if ``OneSpectrumPerFile`` is ``false`` (default value), all spectra will be appended
  into the same file (into different ``<SASdata>`` entries)
* if ``OneSpectrumPerFile`` is ``true``, each spectrum will be written in a separate
  file. The name of the file will be created as follows:

  ``<Filename property>_<spectrum index>_<axis value><axis unit>.<extension>``

The created file can be loaded back into Mantid using the :ref:`algm-LoadCanSAS1D` algorithm.

The canSAS 1-D Format
#####################

The canSAS 1-D standard for reduced 1-D SAS data is implemented using XML
files. A single file can contain SAS data from a single experiment or multiple
experiments.

`Version 1.1`_ of the canSAS 1-D schema can be used to validate files of this format.

There is a `canSAS Wiki`_ available which provides more information about the format.

Versions
########

This is version 2 of the algorithm, which meets version 1.1 of the canSAS 1-D specification.

You can export data to files using `version 1.0`_ of the specification by using :ref:`version 1 of SaveCanSAS1D <algm-SaveCanSAS1D-v1>`.

Usage
-----

**Example - Save/Load "Roundtrip"**

.. testcode:: ExSimpleSavingRoundtrip

   import os

   # Create dummy workspace.
   dataX = [0,1,2,3]
   dataY = [9,5,7]
   out_ws = CreateWorkspace(dataX, dataY, UnitX="MomentumTransfer")

   file_path = os.path.join(config["defaultsave.directory"], "canSASData.xml")

   # Do a "roundtrip" of the data.
   SaveCanSAS1D(out_ws, file_path)
   in_ws = LoadCanSAS1D(file_path)

   print("Contents of the file = " + str(in_ws.readY(0)) + ".")

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the file = [9. 5. 7.].

.. categories::

.. sourcelink::
