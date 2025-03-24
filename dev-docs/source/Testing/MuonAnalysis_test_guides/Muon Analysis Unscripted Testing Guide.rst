.. _Muon_Analysis_TestGuide-ref:

Muon Interfaces Unscripted Testing
==================================

.. contents:: Table of Contents
    :local:

Preamble
^^^^^^^^^
This document is intended for developers to use for unscripted testing of muon GUI's.

The tests follow real use cases provided by scientists and are intended to exercise all the interface's functionality.
As changes are made to the interface and features added, anything for which it is not possible to write an automated
test should have a manual test added to this list. You may need to use the muon feature flags to turn on some additional features (like model analysis, raw plots and fit wizard features). Check the :ref:`Muon_Feature_Flags-ref` documentation for more information.

.. note::
        The tests here are grouped into sections. The test groups can be done in any order.

        In Muon Interfaces, the embedded plot axis are reversed on the plot if ``Xmax < Xmin`` (idem with ``Y`` axis)

Common setup
^^^^^^^^^^^^
- Set your facility to ISIS
- Ensure you can acces the files for the test (listed below).


Group 1: MUSR data
^^^^^^^^^^^^^^^^^^

This group tests data from the MUSR instrument and provides an introduction to the data these tests will be working with (groups and pairs).
The testing instructions can be found at :ref:`Muon_Analysis_MUSR-ref`.

You will need the following runs:

- MUSR 62250-1
- MUSR 62260

Group 2: PSI data
^^^^^^^^^^^^^^^^^

This group tests bin data from the PSI facility and introduces background corrections.
Test instructions can be found at :ref:`Muon_Analysis_PSI-ref`.
You will need the following file from the unit test data:

- deltat_tdc_dolly_1529.bin

Group 3: HIFI data
^^^^^^^^^^^^^^^^^^

This group looks at HIFI data and introduces multiple period data.
Test instructions can be found at :ref:`Muon_Analysis_HIFI-ref`.
You will need the following runs:

- HIFI 134028-39
- HIFI 84447-8

Group 4: EMU data
^^^^^^^^^^^^^^^^^

This group looks at EMU data, it uses some advance fitting and plotting features.
Test instructions for this group can be found at :ref:`Muon_Analysis_EMU-ref`.
You will need the following runs:

- EMU 51341-3
- EMU 20889-20900

Group 5: ARGUS data
^^^^^^^^^^^^^^^^^^^

This group looks at ARGUS data, it explores the difference between single and double pulse data.
Testing instructions are :ref:`Muon_Analysis_ARGUS-ref`.
You will need the following runs:

- ARGUS 71799-800
- ARGUS 71796-7

Group 6: Frequency Domain Analysis
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This group tests the Frequency domain analysis GUI.
Test instructions for group 6 can be found at :ref:`Muon_Analysis_FDA-ref`.
You will need the following run:

- MUSR 62260


Group 7: Avoided Level Crossing (ALC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This group tests the Avoided Level Crossing (ALC) GUI.
Test instructions fcan be found at :ref:`Muon_ALC-ref`.
This requires a large number or runs (close to 100).
Therefore, its best to do these test while connected to the data archive.
