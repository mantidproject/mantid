
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Use this algorithm to translate the geometry of the sample shape defined on a workspace. This algorithm will not change the defined location of the sample object, simply how the physical geometry is defined within the sample's coordinate system

This algorithm works for both CSG shapes
(e.g. cylinders, flat plates etc.) and Mesh files.


Usage
-----
**Example - TranslateSampleShape for sample with a CSG shape**

.. code-block:: python

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   ws1 = CreateSampleWorkspace()
   ws2 = CreateSampleWorkspace()
   ws3 = CreateSampleWorkspace()


   shape_xml = """ <cylinder id="stick">
   <centre-of-bottom-base x="-0.5" y="0.0" z="0.0" />
   <axis x="1.0" y="0.0" z="0.0" />
   <radius val="0.05" />
   <height val="1.0" />
   </cylinder>

   <sphere id="some-sphere">
   <centre x="0.0"  y="0.0" z="0.0" />
   <radius val="0.5" />
   </sphere>

   <algebra val="some-sphere (# stick)" />"""

   SetSampleShape(InputWorkspace = "ws1", ShapeXML = shape_xml)
   SetSampleShape(InputWorkspace = "ws2", ShapeXML = shape_xml)
   SetSampleShape(InputWorkspace = "ws3", ShapeXML = shape_xml)

   # translate shape with comma separated string
   TranslateSampleShape("ws2", "0.0,0.2,0.0")
   # or with sequence of floats
   TranslateSampleShape("ws3", [0.0,0.0,0.3])

.. categories::

.. sourcelink::
