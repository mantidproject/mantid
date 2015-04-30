.. _Create_an_IDF:

Create an IDF
=============

This page will help you get started with making an instrument definition file. For documentation on individual elements of an IDF refer to the :ref:`IDF documentation <InstrumentDefinitionFile>`

Basic Steps
-----------

The recommended set of steps to get started is to:

- Read the introduction section of :ref:`IDF <InstrumentDefinitionFile>`
- Study the annotated IDF examples at the bottom of this page
- Look for similar existing IDFs to the one you would like to create
- Use an editor, which is :ref:`XML schema aware  <Using_XML_Schema>`
- Use the full :ref:`IDF <InstrumentDefinitionFile>` page for detailed documentation

The above are suggestive steps for generating an IDF. In addition there have 
been created several scripts for automatic the creation of IDFs. Such scripts 
can be found `here <https://github.com/mantidproject/mantidgeometry>`__.

Using the Schema
----------------

To set up your editor with the schema, :ref:`Follow These Instructions <Using_XML_Schema>`.
The Schema can help with writing the IDF. For example:

- In Visual Studio the schema can be used to auto-insert elements and attributes permitted by the schema.
- In eclipse the IDF can be created in a design view by selected available elements and attributes without having to type the code yourself.

Find a Similar existing IDF
---------------------------

It may be that an instrument already exists that is similar to the one you wish to add. To see the existing instruments follow the procedure below.

- Install Mantid
- Open MantidPlot
- Execute the algorithm LoadEmptyInstrument
- This open the algorithm window for this algorithm. Click the Browse button and browse to the instrument folder of your Mantid install directory
- The instrument folder contains all the instruments that Mantid currently support. Select for example SANS2D_Definition.xml. Fill in the output workspace name, and and click 'Run'
- The created workspace will appear in the Workspaces window. Right click on the workspace and chose 'Show Instrument'
- A new window appears, which is called the `Instrument view <http://www.mantidproject.org/MantidPlot:_Instrument_View>`__.

In addition below is a list (for now just containing one item) of existing IDFs which have been annotated with the purpose of (hopefully) quickly learn the basis of creating an IDF:

- :ref:`SANS2D: ISIS small angle scattering instrument <SANS2D_Sample_IDF>`
- :ref:`LET: ISIS direct inelastic instrument<LET_Sample_IDF>`

.. categories:: Concepts
