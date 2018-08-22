=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:


ISIS Reflectometry Interface
----------------------------

New
###

- ``SaveMFT`` is a new save option in Ascii format. It can be used instead of the newly deprecated algorithms ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii`` and ``SaveReflThreeColumnAscii``.

New
###

* ``SaveMFT`` is a general algorithm which saves the first spectrum of a workspace in Ascii format particularly suited for reflectometry data.

Improvements
############

- The four Ascii save algorithms ``SaveANSTOAscii``, ``SaveILLCosmosAscii``, ``SaveReflCustomAscii`` and ``SaveReflThreeColumnAscii`` now correctly save x-error and can treat correctly point data and histograms. They are, however, deprecated in favour of ``SaveMFT``. Please see ``SaveMFT`` for more documentation.

Liquids Reflectometer
---------------------

- Default x-direction pixel range for the scaling factor calculation is now set to the full width of the detector as opposed to a restricted guess.

ISIS Reflectometry Interface
----------------------------

Bug fixes
#########

- A bug has been fixed on the Settings tab where the IncludePartialBins check box had been hidden by a misplaced text entry box.

:ref:`Release 3.14.0 <v3.14.0>`
