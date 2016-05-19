.. algorithm::

.. summary::

.. alias::

.. properties::

.. role:: python(code)
   :class: highlight

.. role:: xml(code)
   :class: highlight

Description
-----------

Set properties of the sample & its environment on a workspace.

The 3 arguments to this algorithm ``Environment``, ``Geometry`` & ``Material``
are all expected to be dictionaries specifying multiple parameters that
relate to the respective argument.

Environment
###########

Geometry
########

Specifies the shape of the sample. This container be specified in 1 of 2 ways:

- if an environment is specified that already knows the geometry of the sample
  then the fields of the known geometry container be customized. See :ref:`SampleEnvironment`
  concept page for further details
- a full definition of the shape.

For defining the full shape a key called ``Shape`` specifying the desired shape is 
expected along with additional keys specifying the values (all values are assumed to
be in centimeters):

- ``FlatPlate``: Width, Height, Thick, Center
- ``Cylinder``: Height, Radius, Center, Axis (X=0, Y=1, Z=2)
- ``HollowCylinder``: Height, InnerRadius, OuterRadius, Center, Axis(X=0, Y=1, Z=2)
- ``CSG``: Value is a string containing any generic shape as detailed in 
  :ref:`HowToDefineGeometricShape`

The ``Center`` key is expected to be a list of three values indicating the :python:`[X,Y,Z]`
position of the center.

Material
########

Specifies the composition of the sample using properties from the :ref:`algm-SetSampleMaterial-v1` algorithm.
Please see the algorithm documentation for the supported keywords.

Usage
-----

The following examples assume that a file called ``CRYO-01.xml`` exists in the
``instrument/sampleenvironments/FACILITY/INSTRUMENT/`` directory

.. code-block:: xml

    <!-- Definition of Cryostat 01-->
    <environmentspec>
      <materials>
        <material id="van" formula="V"/>
      </materials>
      <components>
        <containers>
          <container id="8mm" material="van">
            <geometry>
              <hollow-cylinder id="cyla">
                <centre-of-bottom-base x="0.0" y="-0.025" z="0.0"/>
                <axis x="0.0" y="1" z="0" />
                <inner-radius val="0.008" />
                <outer-radius val="0.018" />
                <height val="0.05" />
              </hollow-cylinder>
            </geometry>
            <samplegeometry>
              <cylinder id="cyla">
                <centre-of-bottom-base x="0.0" y="-0.025" z="0.0"/>
                <axis x="0.0" y="1" z="0" />
                <radius val="0.008" />
                <height val="0.05" />
              </cylinder>
            </samplegeometry>
          </container>
        </containers>
      </components>
    </environmentspec>

**Example - container with preset cylinderical sample geometry**

.. test-code:: Ex1

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry as is from environment defintion
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6'})

**Example - Override height of preset cylinder sample**

.. test-code:: Ex2

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set differnet height for sample
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Height': 4.0}
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6'})

**Example - Override complete sample geometry**

.. test-code:: Ex3

   # A fake host workspace, replace this with your real one.
   ws = CreateSampleWorkspace()
   # Use geometry from environment but set differnet height for sample
   SetSample(ws, Environment={'Name': 'CRYO-01', 'Container': '8mm'},
             Geometry={'Shape': 'HollowCylinder', 'Height': 4.0, 
                       'InnerRadius': 0.8, 'OuterRadius': 1.0, 
                       'Center': [0.,0.,0.], 'Axis':1}
             Material={'ChemicalFormula': '(Li7)2-C-H4-N-Cl6'})

.. categories::

.. sourcelink::

