.. _Instrument Data Service:

Instrument Data Service
=======================

What is it?
-----------

The Instrument Data Service (IDS) is a :ref:`Data Service <Data Service>`
that is specialized to hold all of the :ref:`instruments <Instrument>` that
are created during a user session. Whenever an instrument definition is
loaded it is saved in the IDS and further workspaces that refer to the
same instrument share the same definition.

Extracting an instrument from the Instrument Data Service
---------------------------------------------------------

This is rarely something that a user or an algorithm writer would need
to do as it is all handled by the framework internals. Normally you
would access the instrument relating to a workspace directly though that
workspace.

``workspace->getInstrument();``

However if you really did want to access the instrument from the IDS (as
a :ref:`Shared Pointer <Shared Pointer>`), although this would then lack
any workspace specific alterations or properties.

``boost::shared_ptr``\ \ `` intrument = workspace->getInstrument();``



.. categories:: Concepts