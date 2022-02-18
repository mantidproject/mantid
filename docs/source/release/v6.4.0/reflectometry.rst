=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

New features
------------

- On the ISIS Reflectometry interface, you can now specify default experiment settings on a per-sample basis as well as a per-angle basis. The new ``Title`` field in the lookup table allows you to specify a regular expression. Any runs whose title matches this expression will use those settings by default.

Improvements
------------

- Groups are now highlighted to show that all subtasks are completed.

- Processing is disabled if there are errors on the Experiment Setting table.

:ref:`Release 6.4.0 <v6.4.0>`
