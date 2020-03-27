=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

ISIS Reflectometry Interface
############################

Bug fixes
---------

- Save/Load settings: A bug has been fixed where Experiment/Instrument settings were not being restored if the instrument changes on load.
- New Batch loses settings: A bug has been fixed where creating a new Batch would result in the Experiment/Instrument settings of all batches being reset to their defaults.

:ref:`Release 5.1.0 <v5.1.0>`
