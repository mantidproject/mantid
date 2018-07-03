.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. _variety of shapes: http://www.mantidproject.org/HowToDefineGeometricShape

Description
-----------

The algorithm places the user-defined geometric shape within the virtual
instrument and reports back the detector ID of every detector that is
contained within it. Detectors are only considered if their central location
point is contained within the shape.

There are a `variety of shapes`_ available.

Usage
-----

**Example - Finding Detectors Using Cylinders**

.. testcode:: ExDetectorCapture

   import os

   # Load in the MUSR instrument from its IDF file.
   inst_def_file = os.path.join(config["instrumentDefinition.directory"], "MUSR_Definition.xml")
   musr_inst_ws = LoadEmptyInstrument(inst_def_file)

   # Define a cylinder with radius 0.1.
   narrow_cylinder = """
       <infinite-cylinder id=\"shape\">
       <centre x=\"0.0\" y=\"0.0\" z=\"0.0\" />
       <axis x=\"0.0\" y=\"0.0\" z=\"1\" />
       <radius val=\"0.1\" />
       </infinite-cylinder>
       <algebra val=\"shape\" />
       """
   # Define a wider cylinder with radius 0.2.
   wide_cylinder = """
       <infinite-cylinder id=\"shape\">
       <centre x=\"0.0\" y=\"0.0\" z=\"0.0\" />
       <axis x=\"0.0\" y=\"0.0\" z=\"1\" />
       <radius val=\"0.2\" />
       </infinite-cylinder>
       <algebra val=\"shape\" />
       """

   no_detectors = FindDetectorsInShape(musr_inst_ws, narrow_cylinder)
   all_detectors = FindDetectorsInShape(musr_inst_ws, wide_cylinder)

   print("The narrow cylinder contains %i of the detectors." % len(no_detectors))
   print("The wide cylinder contains %i of the detectors." % len(all_detectors))

Output:

.. testoutput:: ExDetectorCapture

   The narrow cylinder contains 0 of the detectors.
   The wide cylinder contains 64 of the detectors.

.. categories::

.. sourcelink::
