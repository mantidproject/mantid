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

An environment definition is contained within a file using XML syntax. The file
can be an explicit definition of the properties of each environment component or
it can contain a reference to a CAD file in .3mf format that contains all the required
information about the environment components

XML Definition File - Explicit Definition
-----------------------------------------

An environment definition is contained within a file using XML syntax.

Multiple containers can be specified in the definition. The correct container for a run
must be chosen by the user at the time the environment is attached to a
workspace with the ``Environment`` option on the
:ref:`SetSample <algm-SetSample>` algorithm.

One Sample Environment can contain components defined in different ways - some components
defined by a CSG geometry and some components
defined in a separate STL mesh file.

CSG geometry
############

A minimal structure with a single container defined using CSG geometry would have the following form:

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
CSG shapes can be plotted in Mantid, see :ref:`Mesh_Plots`.

Mesh files - STL
################

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
- Mantid will then check in the same directory as the environment definition files
- Mantid will then check in the data search directories

The stl file format doesn't natively support a scale so this should be specified
in the scale attribute of the stlfile tag. Possible values are mm, cm or m.

Stl mesh shapes can be plotted in Mantid, see :ref:`Mesh_Plots`.
There are also various free software tools available that can view and edit .stl files:

- FreeCAD (Windows, Linux, Mac). https://www.freecadweb.org/ This viewer also provides coordinate readout of the cursor position
- Microsoft 3D Viewer (Windows only)

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
- ``attenuationprofile``

Mantid will search for the filename supplied in the attenuationprofile attribute in the
following places (in order):

- If a full path is supplied in the filename attribute then it will be used
- Mantid will then check in the same directory as the environment definition file
- Mantid will then check in the data search directories

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
in degrees.

XML Definition File - 3MF Definition
-----------------------------------------

The .3mf file format is a 3D printing format that allows multiple meshes with their relative orientations to be stored in a single file along with information on the scale
used for vertex coordinates and metadata about the material properties. Further details on the format are available here:

https://3mf.io/

If all the information on the geometry of the environment components is available in a single .3mf file this can be referenced in the sample environment xml file instead
of supplying the full details as described above.

The following xml example shows this type of reference:

.. code-block:: xml

    <!-- Filename: 3MFExample.xml -->
    <environmentspec>
      <fullspecification filename="Assembled.3mf"/>
    </environmentspec>

If a relative path or filename is supplied for the 3mf file name, Mantid searches in the same set of
directories that are described above for .stl files.

The materials must have their names set to the material's chemical formula in order for the material
data to be imported into Mantid. Additional properties such as the density should be specified in brackets
after the name:

eg B4-C (massdensity='2.52', cohscatterxsec='10')

While there are a wide range of CAD tools available that support import and export from .3mf format,
support for saving material information into .3mf format is more limited. The material information
can be easily added to the .3mf files however by editing the file in a text editor:

- change the .3mf file extension to .zip
- extract the file called 3dmodel.model
- edit the <basematerials> content near the top of the file

The 3mf file can optionally include the geometry of the sample as well as the environment. The mesh
corresponding to the sample should be given the name 'sample' in the 3mf file.

.. categories:: Concepts
