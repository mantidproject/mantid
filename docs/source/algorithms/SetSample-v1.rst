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

Set properties of the sample & its environment on a workspace.

The 3 arguments to this algorithm ``Environment``, ``Geometry`` and
:py:obj:`Material <mantid.kernel.Material>` are all expected to be
dictionaries specifying multiple parameters that relate to the
respective argument.

.. note:: Contrary to the :ref:`xml forms of defining the geometry
          <HowToDefineGeometricShape>` which are in metres,
          :py:obj:`dict` versions are in centimetres.

Environment
###########

Specifies the sample environment kit to be used. There are two required keywords:

- ``Name``: The name of the predefined kit
- ``Container``: The id of the container within the predefined kit

See :ref:`SampleEnvironment` concept page for further details on how the creating
a definition file.

The name of a kit is must be unique for a given instrument. The following
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


Geometry
########

Specifies the shape of the sample. This can be specified in 1 of 2 ways:

- if an environment is specified that already knows the geometry of the sample
  then the fields of the known geometry container be customized. See :ref:`SampleEnvironment`
  concept page for further details
- a full definition of the shape.

For defining the full shape a key called ``Shape`` specifying the desired shape is
expected along with additional keys specifying the values (all values are assumed to
be in centimeters):

- ``FlatPlate``: Width, Height, Thick, Center, Angle
- ``Cylinder``: Height, Radius, Center
- ``HollowCylinder``: Height, InnerRadius, OuterRadius, Center
- ``CSG``: Value is a string containing any generic shape as detailed in
  :ref:`HowToDefineGeometricShape`

The ``Center`` key is expected to be a list of three values indicating the :python:`[X,Y,Z]`
position of the center. The reference frame of the defined instrument is used to
set the coordinate system for the shape.

The ``Angle`` argument for a flat plate shape is expected to be in degrees and is defined as
the angle between the positive beam axis and the normal to the face perpendicular to the
beam axis when it is not rotated, increasing in an anti-clockwise sense. The rotation is
performed about the vertical axis of the instrument's reference frame.

Material
########

Specifies the composition of the sample using properties from the :ref:`algm-SetSampleMaterial-v1` algorithm.
Please see the algorithm documentation for the supported keywords.

Usage
-----

The following example uses a test file called ``CRYO-01.xml`` in the
``[INSTALLDIR]/instrument/sampleenvironments/TEST_LIVE/ISIS_Histogram/`` directory.

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
                       'SampleNumberDensity': 0.1})

**Example - Override height of preset cylinder sample**

.. testcode:: Ex2

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set different height for sample
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Height': 4.0},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'SampleNumberDensity': 0.1})

**Example - Specify height and mass of preset cylinder sample**

.. testcode:: Ex2

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set different height for sample
   # and calculate density with supplied sample mass
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Height': 4.0},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6',
                       'SampleMass': 3.0})

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
                       'SampleNumberDensity': 0.1})

.. categories::

.. sourcelink::
