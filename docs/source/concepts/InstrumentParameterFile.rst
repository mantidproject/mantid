.. _InstrumentParameterFile:

.. role:: xml(literal)
   :class: highlight

Instrument Parameter File
=========================

Summary
-------

Instrument parameter files are files complimentary to :ref:`Instrument Definition Files (IDFs)<InstrumentDefinitionFile>` that are used to store information about an instrument that may change on a regular basis i.e. parameters not related to the geometry of an instrument. By storing the data outside of the :ref:`IDF<InstrumentDefinitionFile>`, it makes it easier to locate and change parameters, as well as keeping the length of the :ref:`IDF<InstrumentDefinitionFile>` to a minimum.

Creating a Parameter File
-------------------------

Using a Schema
~~~~~~~~~~~~~~

To create a parameters file it is advisable to consult the parameter file schema, located in your mantid directory at mantid\code\instrument\Schema\ParameterFileSchema.xsd. Set up your editing program to validate your XML file against this schema following :ref:`these instructions <Using_XML_Schema>`. Once set up, the schema can be used to find any errors in the structure of your parameter file and suggest auto-fill options to help write your parameter file.

General Structure
~~~~~~~~~~~~~~~~~

The Instrument parameter files, like the :ref:`IDFs<InstrumentDefinitionFile>`, are written in XML. They must contain a root element <parameter-file> with an attribute 'name' equal to the name of the instrument. Within the <parameter-file> element, goes all the rest of the information. To specify a parameter for a component defined the :ref:`IDF<InstrumentDefinitionFile>`, use a <component-link> element with attribute 'name' equal to the name of the component. Within the <component-link> element, the various parameters can be defined mostly in the same way as described in the parameters section of the IDF Page. It is possible to also specify 'visible' attribute, which can used to define whether the parameter specified for the component is printed in the pick tab of the InstrumentViewer.
It is possible to set multiple values for a parameter, each one with a defined time range.
The below example shows some of the elements featured in IN10_silicon_111_Parameters.xml, slightly modified. It defines a parameter 'analysis-type' for the component IN10 i.e. the whole instrument. The component link is closed and then a new one is opened to define parameters for the component 'silicon'.


.. code-block:: xml

  <?xml version="1.0" encoding="UTF-8" ?>
  <parameter-file instrument="IN10" date="2010-07-15 00:00:00">

    <component-link name="IN10">
      <parameter name="analysis-type" type="string">
        <value val="spectroscopy" />
      </parameter>
    </component-link>

    <component-link name="silicon">
      <parameter name="Efixed" visible="true">
        <value val="2.0" valid-to="2011-12-31T23:59:59"/>
        <value val="2.082" valid-from="2012-01-01T00:00:00" valid-to="2012-12-31T23:59:59"/>
        <value val="3.0" valid-from="2013-01-01T00:00:00"/>
      </parameter>
    </component-link>

  </parameter-file>

Naming and Using a Parameter File
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There may be just one parameter file per instrument that is automatically included with name the form XXX_Parameters.xml, for instrument XXX in the instrument folder.

One can have several parameter files for an instrument:

- If the IDF is not in the instrument folder and there is another XXX_Parameters.xml in the same folder, this one in the same folder will be used instead of any parameter file in the instrument folder.
- If you want one parameter file for your IDF file, name your IDF file XXX_Definition_Yyy.xml and the parameter file XXX_Parameters_Yyy.xml , where Yyy is any combination a characters you find appropriate. If your IDF file is not in the instrument folder, the parameter file can be in either the same folder or in the instrument folder, but it can only be in the instrument folder, if the same folder has no XXX_Parameters.xml or XXX_Parameters_Yyy.xml file.
- If there is no XXX_Parameters_Yyy.xml file, XXX_Parameters.xml would be used.

Also one can use a any parameter file by executing the :ref:`LoadParameterFile <algm-LoadParameterFile>` algorithm.

Work is planned to enable a parameter file to import one other parameter file. This will then enable duplication to be avoided in the parameter files.

Adding Parameters at run time
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Since version 3.2 of Mantid the algorithm :ref:`SetInstrumentParameter<algm-SetInstrumentParameter>` allows you to create or replace instrument parameters for a particular workspace at run time.

.. categories:: Concepts
