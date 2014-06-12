.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Saves the given workspace to a file which will be in canSAS 1-D format
specified by canSAS 1-D Data Formats Working Group schema
http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd. CANSAS
has a Wiki page at
http://www.smallangles.net/wgwiki/index.php/canSAS_Working_Groups.

The second version is compatible with canSAS - 1D - Version 1.1. If you
need to save to the version 1.0 please use the first version of this
algorithm. You can see it at: [SaveCanSAS1D\_1.0].

Workspace group members and appended workspaces are stored in separate
SASentry `xml <http://en.wikipedia.org/wiki/Xml>`__ elements.

This algorithm saves workspace into CanSAS1d format. This is an xml
format except the , tags and all data in between must be one line, which
necesitates the files be written iostream functions outside xml
libraries.

The second version of CanSAS1D implements the version 1.1, whose schema
is found at http://www.cansas.org/formats/1.1/cansas1d.xsd. See the
tutorial for more infomation about:
http://www.cansas.org/svn/1dwg/trunk/doc/cansas-1d-1_1-manual.pdf.

The structure of CanSAS1d xml is defined at the links above.

.. categories::
