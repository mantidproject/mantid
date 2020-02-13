.. _SampleEnvironment:

==================
Sample Environment
==================

.. role:: xml(literal)
   :class: highlight

A sample environment defines the container + components used to hold a sample
during a run. This page details the process of a sample environment within
Mantid.

Specification
-------------

A sample environment is defined by:

- one or more available containers, each with a defined :ref:`geometry
  <HowToDefineGeometricShape>` and :ref:`composition <Materials>`
- optional additional components that will be in the beam, each with
  their own :ref:`geometry <HowToDefineGeometricShape>` and
  :ref:`composition <Materials>`

At a minimum a sample environment is expected to define a container with both its
geometry and composition.

XML Definition File
-------------------

An environment definition is contained within a file using XML syntax. A minimal
structure with a single container defined using CSG geometry would have the following form:

.. code-block:: xml

    <!-- Filename: CRYO-01.xml -->
    <environmentspec>
      <materials>
        <material id="vanadium" formula="V"/>
      </materials>
      <components>
        <containers>
          <container id="10mm" material="vanadium">
            <geometry>
            <!-- geometry of container -->
            </geometry>
            <samplegeometry>
            <!-- geometry of sample -->
            </samplegeometry>
          </container>
        </containers>
      </components>
    </environmentspec>

The CSG geometry of both the sample and container are defined using the same syntax
used in the instrument definition files to define detector shapes. See
:ref:`here <HowToDefineGeometricShape>` for detail on defining shapes in XML.

Multiple containers container be specified in the definition. The correct container for a run
must be chosen by the user at the time the environment is attached to a
workspace with the ``Environment`` option on the
:ref:`SetSample <algm-SetSample>` algorithm.

The container and sample geometry can alternatively be defined using a mesh description by 
specifying an .stl file as follows.

.. code-block:: xml

    <environmentspec>
      <materials>
        <material id="vanadium" formula="V"/>
      </materials>
      <components>
        <containers>
          <container id="10mm" material="vanadium">
            <stlfile filename="container.stl" scale="mm">
            </stlfile>
            <samplestlfile filename="sample.stl" scale="mm">
            </samplestlfile>
          </container>
        </containers>
      </components>
    </environmentspec>
	
Mantid will try the following approaches to find the path to the stl file (in order):

- If a full path is supplied in the filename attribute then it will be used
- Mantid will then check in the same directory as the environment definition file
- Mantid will then check in the data search directories

The stl file format doesn't natively support a scale so this should be specified
in the scale attribute of the stilfile tag. Possible values are mm, cm or m.

Materials
#########

Each component is assigned a material, which defines properties such as the
number density and neutron scattering cross sections, amongst other things.
All materials defined for an environment must be defined within the :xml:`<materials>`
tags and each material must have a unique :xml:`id` within the file. The :xml:`id`
is used to reference the material when defining a container or component.

The other attributes define the properties of the material. The allowed attributes
map to the arguments of a similar name on the :ref:`SetSampleMaterial <algm-SetSampleMaterial>` algorithm

- ``formula``
- ``atomicnumber``
- ``massnumber``
- ``numberdensity``
- ``zparameter``
- ``unitcellvol``
- ``massdensity``
- ``totalscatterxsec``
- ``cohscatterxsec``
- ``incohscatterxsec``
- ``absorptionxsec``

Non-container Components
------------------------

A given setup may have other components within the beam that must be included. These
container be included using the :xml:`component` tag rather than the :xml:`container` tag. For
example, a heat shield container be added to the above definition like so:

.. code-block:: xml

    <!-- Filename: CRYO-01.xml -->
    <environmentspec>
      <materials>
        <material id="vanadium" formula="V"/>
        <material id="aluminium" formula="Al"/>
      </materials>
      <components>
       <containers>
        <container id="10mm" material="vanadium">
         <geometry>
          <!-- geometry of container -->
         </geometry>
         <samplegeometry>
          <!-- geometry of sample -->
         </samplegeometry>
        </container>
       </containers>
       <component id="heat-shield" material="aluminium">
        <geometry>
         <!-- geometry of shield-->
        </geometry>
       </component>
      </components>
    </environmentspec>

A new material, ``aluminium`` has been added to the materials list and the heat shield
is defined as an arbitrary :xml:`component`. The :xml:`component` tag behaves in a similar fashion to
the :xml:`container` tag with the exception that it container not contain a :xml:`samplegeometry`.

The non-container components can also be defined using mesh geometry by specifying stl file names

.. code-block:: xml

    <!-- Filename: CRYO-01.xml -->
    <environmentspec>
      <materials>
        <material id="vanadium" formula="V"/>
        <material id="aluminium" formula="Al"/>
      </materials>
      <components>
        <containers>
          <container id="10mm" material="vanadium">
            <stlfile filename="container.stl" scale="mm">
            </stlfile>
            <samplestlfile filename="sample.stl" scale="mm">
            </samplestlfile>
          </container>
        </containers>
        <component id="heat-shield" material="aluminium">
          <stlfile filename="heat-shield.stl" scale="mm">
            <translation vector="0,0,1.40384"/>
            <rotation ydegrees="180"/>
          </stlfile>
        </component>
      </components>
    </environmentspec>
	
The shape defined in the stl file can be transformed and\or rotated in order to assemble it correctly with
the other environment components. This is achieved by specifying a translation or rotation tag in the xml.
The translation tag has an attribute vector which is a comma separated list of x, y, z coordinates.
The rotation tag has available attributes xdegrees, ydegrees, zdegrees which all take a rotation specified
in degrees

.. categories:: Concepts
