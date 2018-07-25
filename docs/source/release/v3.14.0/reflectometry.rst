=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:


ISIS Reflectometry Interface
----------------------------

New
###

- ``SaveMFT`` is a new save option in Ascii format. It can be used instead of the newly deprecated algorithms ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii`` and ``SaveReflsThreeColumnAscii``.

New
###

* ``SaveMFT`` is a general algorithm which saves the first spectrum of a workspace in Ascii format particularly suited for reflectometry data.

Improvements
############

- The four Ascii save algorithms ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii`` and ``SaveReflsThreeColumnAscii`` now correctly save x-error and can treat correctly point data and histograms. They are, however, deprecated in favour of ``SaveMFT``. Please see ``SaveMFT`` for more documentation.

:ref:`Release 3.14.0 <v3.14.0>`
