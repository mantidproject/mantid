.. _Using_XML_Schema:

Using XML Schemas
=================

This page gives instructions on how to configure various editing programs for 
writing XML with schema validation. This helps by finding errors in 
XML structure, and displaying the list of elements and attributes allowed at 
any point in the XML file. This will be useful for anyone creating an 
Instrument Definition file (IDF) or Parameters file, or editing the Facilities file

XML Editors with Schema Validation tools
----------------------------------------
Examples of editors that include schema validation tools:

- `Visual Studio <http://www.visualstudio.com/en-us>`__
- `Eclipse <http://www.eclipse.org>`__
- `jEdit <http://www.jedit.org>`__
- `Altova <http://www.altova.com/xmlspy/xml-schema-editor.html>`__
- `Liquid Tehnologies <http://www.liquid-technologies.com/xml-schema-editor.aspx>`__
- `Stylusstudio <http://www.stylusstudio.com/open_xsd_validation.html>`__

Where an XML includes a link to an XML schema then simply open the XML file in
say Visual Studio and the editor will give you feedback if you for example are 
missing attributes of an XML element that the Schema thinks should be there.  

For example all :ref:`IDFs <InstrumentDefinitionFile>` shipped with Mantid
contain a link to the IDF Schema. 

.. categories:: Concepts
