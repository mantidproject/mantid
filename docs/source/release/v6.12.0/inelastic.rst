=================
Inelastic Changes
=================

.. contents:: Table of Contents
   :local:

New Features
------------
- The deprecated Calculate Paalman Pings tab on the :ref:`Inelastic Corrections interface <interface-inelastic-corrections>` has been removed after no user feedback from v6.10 and v6.11.
- The :ref:`Data Processor Interface <interface-inelastic-data-processor>` :math:`S(Q, \omega)` tab can now load ILL data.


Bugfixes
--------
- :ref:`Data Processor Interface <interface-inelastic-data-processor>` :ref:`Elwin Tab <elwin>`:

  - Integration range automatically updates as expected when workspaces are added via the ``Add Data`` button.
  - Plot generation from workspaces ending with ``elwin_elf`` now works correctly and displays no overflow warning messages.


:ref:`Release 6.12.0 <v6.12.0>`
