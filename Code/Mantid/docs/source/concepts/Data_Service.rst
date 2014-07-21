.. _Data Service:

Data_Service
============

What are they?
--------------

Data Services are the internal storage locations within Mantid. Each
data service holds a list of objects that share a common base class (for
example the Instrument Data Service can hold anything which inherits
from instrument). Each item that is held is associated with a name that
uniquely describes the object. This name is them used when the object
needs to be retrieved, or for other operations such as overwriting it or
deleting it.

Data Services in Mantid
-----------------------

-  `Analysis Data Service <Analysis Data Service>`__ - A data service
   holding all of the `workspaces <Workspace>`__ used in this session.
-  `Instrument Data Service <Instrument Data Service>`__ - A data
   service holding all of the `instruments <Instrument>`__ used in this
   session.



.. categories:: Concepts