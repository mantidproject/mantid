=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Instrument & IDF
----------------

- New CRISP filename format, full name + 8 digits, is now recognised
- POLREF IDF has been updated
- OFFSPEC IDF has been updated

ReflectometryReductionOne
-------------------------

- The rebinning in wavelength has been removed

ReflectometryReductionOneAuto
-----------------------------

- The scaling step has now been added to the Workflow diagram of ReflectometryReductionOneAuto `#16671 <https://github.com/mantidproject/mantid/pull/16671>`__   

CreateTransmissionWorkspace
---------------------------

- A workflow diagram has been added to the documentation.
- The rebinning in wavelength has been removed

ISIS Reflectometry (Polref)
###########################

- Interface now displays information in a tree where groups are parent items and runs are children. For more details, please check the updated documentation.
- Global settings have been moved to a separate tab ("Settings")
- Transfer progress bar no longer gives impression of running when clicked if no runs are selected
- Updated instrument definition files.
- Files are now loaded into the interface using LoadISISNexus

`Full list of changes on github <http://github.com/mantidproject/mantid/pulls?q=is%3Apr+milestone%3A%22Release+3.8%22+is%3Amerged+label%3A%22Component%3A+Reflectometry%22>`__
