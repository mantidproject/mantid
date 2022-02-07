
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is intended to write out portions of an instrument's
geometry in the :ref:`Instrument Definition File <InstrumentDefinitionFile>`
xml format. The resulting file is meant to be copied by-hand into a
geometry file, the output is not a useable IDF as written.

The main use of this algorithm is if the instrument geometry is
calibrated in mantid, this algorithm can be used to help get the
information back into the initial instrument definition file.

Usage
-----

.. testcode:: ExportGeometry

   LoadEmptyInstrument(Filename="NOMAD_Definition.xml",
                       OutputWorkspace="NOM_geom")
   import mantid
   filename=mantid.config.getString("defaultsave.directory")+"NOMgeometry.xml"
   ExportGeometry(InputWorkspace="NOM_geom",
                  Components="bank46,bank47",
                  Filename=filename)
   import os
   if os.path.isfile(filename):
       print("File created: True")

.. testcleanup:: ExportGeometry

   DeleteWorkspace("NOM_geom")
   import mantid
   filename=mantid.config.getString("defaultsave.directory")+"NOMgeometry.xml"
   import os
   if os.path.isfile(filename):
       os.remove(filename)


Output:

.. testoutput:: ExportGeometry

   File created: True

.. categories::

.. sourcelink::
