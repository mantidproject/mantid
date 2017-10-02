=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

Data Objects
------------

Python
------
In `mantid.simpleapi`, a keyword has been implemented for function-like algorithm calls to control the storing on the Analysis Data Service.
`StoreInADS=False` can be passed to function calls of child algorithms to not to store their output on the ADS.

:ref:`Release 3.12.0 <v3.12.0>`
