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
structure with a single container would have the following form:

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

The geometry of both the sample and container are defined using the same syntax
used in the instrument definition files to define detector shapes. See
:ref:`here <HowToDefineGeometricShape>` for detail on defining shapes in XML.

Multiple containers container be specified in the definition. The correct container for a run
must be chosen by the user at the time the environment is attached to a
workspace with the ``Environment`` option on the
:ref:`SetSample <algm-SetSample>` algorithm.

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
         <!-- geometry of sheild-->
        </geometry>
       </component>
      </components>
    </environmentspec>

A new material, ``aluminium`` has been added to the materials list and the heat shield
is defined as an arbitrary :xml:`component`. The :xml:`component` tag behaves in a similar fashion to
the :xml:`container` tag with the exception that it container not contain a :xml:`samplegeometry`.

.. categories:: Concepts
