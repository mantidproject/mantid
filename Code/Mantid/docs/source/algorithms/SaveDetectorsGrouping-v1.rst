.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to save a GroupingWorkspace to a file in XML
format.

XML File Format
---------------

Parameters
##########

-  "instrument": mandatory attribute of node 'detector-grouping'. It
   must be valid instrument name.
-  "ID": mandatory attribute of node 'group'. It must be valid group
   name, and the key to denote group.
-  "detids": a node to define grouping by detectors' ID. Its value must
   be a list of integers separated by ','. A '-' is used between 2
   integers to define a range of detectors.
-  "component": a node to define that all detectors belonged to a
   component in the instrument are to be in a same group. Its value
   should be a valid component name.

Example 1:

.. raw:: html

   <?xml version="1.0" encoding="UTF-8" ?>

| `` ``\ 
| ``  ``\ 
| ``   ``\ \ ``1-30,34-44,47-100``\ 
| ``  ``\ 
| ``   ``\ \ ``103-304,344-444,474-5000``\ 
| ``  ``\ 
| `` ``\

.. categories::
