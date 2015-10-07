.. _Data Service:

Data Service
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

-  :ref:`Analysis Data Service <Analysis Data Service>` - A data service
   holding all of the :ref:`workspaces <Workspace>` used in this session.
-  :ref:`Instrument Data Service <Instrument Data Service>` - A data
   service holding all of the :ref:`instruments <Instrument>` used in this
   session.



.. categories:: Concepts