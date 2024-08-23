.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

.. role:: python(code)
   :class: highlight

.. role:: xml(code)
   :class: highlight

Description
-----------

Set properties of the sample and its environment on a workspace.

The arguments to this algorithm are all expected to be
dictionaries specifying multiple parameters that relate to the
respective argument, as explained below.

.. note:: Contrary to the :ref:`xml forms of defining the geometry <HowToDefineGeometricShape>` which are in metres,
          :py:obj:`dict` versions are in centimetres.

Geometry and ContainerGeometry
##############################

Specifies the shape of the sample (and container). This can be specified in the following ways:

- a sample geometry can be defined in the environment definition file using either CSG or Mesh geometry.
  If this approach is taken then the Geometry property can be left blank
- if an environment is specified that already knows the geometry of the sample and that geometry
  is defined using a CSG shape (ie non-mesh shape) then the fields of the known geometry container
  can be customized. See :ref:`SampleEnvironment` concept page for further details
- a full definition of the shape can be supplied in this property.

For defining the full shape a key called ``Shape`` specifying the desired shape is
expected along with additional keys specifying the values (all values are assumed to
be in centimeters):

- ``FlatPlate``: Width, Height, Thick, Center, Angle
- ``Cylinder``: Height, Radius, Center, Axis
- ``HollowCylinder``: Height, InnerRadius, OuterRadius, Center, Axis
- ``FlatPlateHolder``: Width, Height, Thick, Center, Angle, FrontThick, BackThick. This is a CSG union of 2 FlatPlates tightly wrapping a FlatPlate sample. To be used for the ContainerGeometry.
- ``HollowCylinderHolder``: Height, InnerRadius, InnerOuterRadius, OuterInnerRadius, OuterRadius, Center. This is a CSG union of 2 HollowCylinders tightly wrapping a HollowCylinder sample. To be used for the ContainerGeometry.
- ``Sphere``: Center, Radius
- ``CSG``: Value is a string containing any generic shape as detailed in :ref:`HowToDefineGeometricShape`

The ``Center`` key is expected to be a list of three values indicating the :python:`[X,Y,Z]`
position of the center, which would be the geometrical center of the shape.
The reference frame of the defined instrument is used to
set the coordinate system for the shape.

The ``Angle`` argument for a flat plate shape is expected to be in degrees and is defined as
the angle between the positive beam axis and the normal to the face perpendicular to the
beam axis when it is not rotated, increasing in an anti-clockwise sense. The rotation is
performed about the vertical axis of the instrument's reference frame.

Material and ContainerMaterial
##############################

Specifies the composition of the sample (or its container) using properties from the :ref:`algm-SetSampleMaterial-v1` algorithm.
Please see the algorithm documentation for the supported keywords.

.. note:: Note that for the keys which historically had the **Sample** prefix (e.g. SampleNumberDensity) the prefix should not be specified here; that is, **NumberDensity** instead of **SampleNumberDensity**, etc. However, for backwards compatibility, it works also with prefixes.

.. note:: Note that this algorithm does not invoke :ref:`algm-SetSampleMaterial-v1` anymore, but sets the material directly through the API.


Environment
###########

Specifies the sample environment kit to be used. There are two possibilities:

Environment Definition File
^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this case the environment kit must be defined in the XML format. See :ref:`SampleEnvironment` concept page for further details on how the creating
a definition file.

Three keywords must be specified in the ``Environment`` dictionary:

- ``Name``: The name of the predefined kit (required)
- ``Container``: The id of the container within the predefined kit. (required if there is more than one container defined for the kit).
- ``Path``: The directory containing the predefined kit, allowing XML files to be loaded from any directory.
  If this is not set, then the Sample Environment XML file must be in one of the instrument directories.

The name of a kit must be unique for a given instrument. The following
procedure is used when trying to find a named definition, e.g ``CRYO-01``:

- check the instrument name on the input workspace:

  - if this is a known instrument at a known facility (is in Facilities.xml) then
    use these as ``FACILITY`` & ``INSTRUMENT`` respectively

  - else use the default facility and instrument as ``FACILITY`` & ``INSTRUMENT`` respectively

- append ``.xml`` to the given kit name

- find the current list of directories containing instrument definition files
  (see :ref:`Instrument Definition Directories <InstrumentDefinitionFile_Directories>`
  for the default directory list)

- for each (``INSTDIR``) in turn:

  - construct a test path ``INSTDIR/sampleenvironments/FACILITY/INSTRUMENT/CRYO-01.xml``

  - if this file exists then select this as the kit file and the search stops

  - otherwise if the file does not exist continue onto the next ``INSTDIR``

  - repeat for the facility directories if not found in for the specific instrument

Container Geometry and Material
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can specify the geometry and the material of a single container directly with the ContainerGeometry and ContainerMaterial dictionaries.
This option is used only when Environment input is left blank. See the sections above for the available keywords to configure those.

Usage
-----

The following example uses a test file called ``CRYO-01.xml`` in the
``[INSTALLDIR]/instrument/sampleenvironments/TEST_LIVE/ISIS_Histogram/`` directory.

If the examples are run via the Mantid user interface then double instead of single quotes will need to be
used for the dictionary parameters.

**Example - Container with preset cylinderical sample geometry**

.. testsetup:: *

   FACILITY_AT_START = config['default.facility']
   INSTRUMENT_AT_START = config['default.instrument']
   config['default.facility'] = 'TEST_LIVE'
   config['default.instrument'] = 'ISIS_Histogram'

.. testcleanup:: *

   config['default.facility'] = FACILITY_AT_START
   config['default.instrument'] = INSTRUMENT_AT_START

.. testcode:: Ex1

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()

   # Use geometry as is from environment definition
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'NumberDensity': 0.1})

**Example - Override height of preset cylinder sample**

.. testcode:: Ex2

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set different height for sample
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Height': 4.0},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'NumberDensity': 0.1})

**Example - Specify height and mass of preset cylinder sample**

.. testcode:: Ex2

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set different height for sample
   # and calculate density with supplied sample mass
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Height': 4.0},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'Mass': 3.0})

**Example - Override complete sample geometry**

.. testcode:: Ex3

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set different height for sample
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Shape': 'HollowCylinder', 'Height': 4.0,
                       'InnerRadius': 0.8, 'OuterRadius': 1.0,
                       'Center': [0.,0.,0.]},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'NumberDensity': 0.1})

**Example - Use sphere sample geometry**

.. testcode:: Ex4

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()

   # Set sample geometry of workspace to a Sphere
   SetSample(ws, Geometry={'Shape': 'Sphere',
                 'Radius': 2.0, 'Center': [0.,0.,0.]})

**Example - Flat plate sample in a flat plate holder container**

.. testcode:: Ex5

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   SetSample(ws,
           Geometry={'Shape': 'FlatPlate', 'Height': 4.0,
                     'Width': 2.0, 'Thick': 1.0,
                     'Center': [0.,0.,0.]},
           Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                     'NumberDensity': 0.1},
           ContainerGeometry={'Shape': 'FlatPlateHolder', 'Height': 4.0,
                     'Width': 2.0, 'Thick': 1.0, 'FrontThick': 0.3, 'BackThick': 0.4,
                     'Center': [0.,0.,0.]},
           ContainerMaterial={'ChemicalFormula': 'Al',
                     'NumberDensity': 0.01})

**Example - Cylinder sample in a hollow cylinder container**

.. testcode:: Ex6

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   SetSample(ws,
           Geometry={'Shape': 'Cylinder', 'Height': 4.0,
                     'Radius': 2.0, 'Center': [0.,0.,0.]},
           Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                     'NumberDensity': 0.1},
           ContainerGeometry={'Shape': 'HollowCylinder', 'Height': 4.0,
                     'InnerRadius': 2.0, 'OuterRadius': 2.3,
                     'Center': [0.,0.,0.]},
           ContainerMaterial={'ChemicalFormula': 'Al',
                     'NumberDensity': 0.01})

**Example - Hollow cylinder sample in a hollow cylinder holder container**

.. testcode:: Ex7

  # A fake host workspace, replace this with your real one.
  ws = CreateSampleWorkspace()
  SetSample(ws,
          Geometry={'Shape': 'HollowCylinder', 'Height': 4.0,
                    'InnerRadius': 2.0, 'OuterRadius': 3.0, 'Center': [0.,0.,0.]},
          Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                    'NumberDensity': 0.1},
          ContainerGeometry={'Shape': 'HollowCylinderHolder', 'Height': 4.0,
                    'InnerRadius': 1.5, 'InnerOuterRadius': 2.0, 'OuterInnerRadius': 3.0, 'OuterRadius': 4.0,
                    'Center': [0.,0.,0.]},
          ContainerMaterial={'ChemicalFormula': 'Al',
                    'NumberDensity': 0.01})

**Example - Specify shape using CSG object**

.. testcode:: Ex8

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Specify an Infinite Cylinder geometry using CSG
   infinite_cylinder_xml = " \
   <infinite-cylinder id='some-cylinder'> \
       <centre x='0.0'  y='0.2' z='0' /> \
	   <axis x='0.0'  y='0.2' z='0' /> \
       <radius val='1' /> \
   </infinite-cylinder> \
   <algebra val='some-cylinder' /> \
   "
   # Set sample geometry of workspace to this CSG object Sphere
   SetSample(ws, Geometry={'Shape': 'CSG', 'Value': infinite_cylinder_xml})

**Example - SetGoniometer to apply automatic rotation to Sample Shape.**

SetSample can be used to apply an automatic goniometer rotation. :ref:`SetGoniometer <algm-SetGoniometer>` should be called before SetSample.
After running this example code to rotate a cuboid by 30° anti-clockwise around y, the sample shape can be plotted (see :ref:`Mesh_Plots`):

.. code-block:: python

    cuboid = " \
    <cuboid id='some-cuboid'> \
    <height val='2.0'  /> \
    <width val='2.0' />  \
    <depth  val='0.2' />  \
    <centre x='10.0' y='10.0' z='10.0'  />  \
    </cuboid>  \
    <algebra val='some-cuboid' /> \
    "

    ws = CreateSampleWorkspace()
    SetGoniometer(ws, Axis0="30,0,1,0,-1")
    SetSample(ws, Geometry={'Shape': 'CSG', 'Value': cuboid})

.. plot::

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   from mpl_toolkits.mplot3d.art3d import Poly3DCollection

   cuboid = " \
   <cuboid id='some-cuboid'> \
   <height val='2.0'  /> \
   <width val='2.0' />  \
   <depth  val='0.2' />  \
   <centre x='10.0' y='10.0' z='10.0'  />  \
   </cuboid>  \
   <algebra val='some-cuboid' /> \
   "

   ws = CreateSampleWorkspace()
   SetGoniometer(ws, Axis0="30,0,1,0,-1")
   SetSample(ws, Geometry={'Shape': 'CSG', 'Value': cuboid})

   sample = ws.sample()
   shape = sample.getShape()
   mesh = shape.getMesh()

   facecolors = ['purple','mediumorchid','royalblue','b','red','firebrick','green', 'darkgreen','grey','black', 'gold', 'orange']

   mesh_polygon = Poly3DCollection(mesh, facecolors = facecolors, linewidths=0.1)

   fig, axes = plt.subplots(subplot_kw={'projection':'mantid3d'})
   axes.add_collection3d(mesh_polygon)

   axes.set_title('Sample Shape: Cuboid ws @ 30°')
   axes.set_xlabel('X / m')
   axes.set_ylabel('Y / m')
   axes.set_zlabel('Z / m')

   axes.set_mesh_axes_equal(mesh)
   axes.view_init(elev=20, azim=80)

   fig.show()

.. categories::

.. sourcelink::
