Indirect Diffraction Testing
============================

.. contents::
   :local:

Diffraction
-----------

*Preparation*

-  You need access to the ISIS data archive
- ``osiris_041_RES10.cal`` file from the unit test data

**Time required 2-5 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Diffraction``
#. Make sure ``Instrument`` is set to ``IRIS``
#. The default Detector Grouping should be ``All``
#. In ``Run Numbers`` enter 26176
#. Click ``Run``
#. The output workspace should have 1 spectra
#. Change the Detector Grouping to ``Groups`` and enter 4 groups
#. Click ``Run``
#. The output workspace should have 4 spectra
#. Change the Detector Grouping to ``Custom``, enter ``105-108,109-112``
#. Click ``Run``
#. The output workspace should have 2 spectra
#. Still using ``Custom``, try spectra numbers outside of the range 105-112. This should not be allowed.
#. Using ``Groups``, try more than 8 groups. This should not be allowed.
#. Using ``File``, try an empty string. This should not be allowed.

**Time required 2-5 minutes**

--------------

#. Open ``Interfaces`` > ``Indirect`` > ``Diffraction``
#. Make sure ``Instrument`` is set to ``OSIRIS``
#. Set ``Reflection`` to ``Diffonly``
#. In ``Run Numbers`` enter 89813
#. In ``Vanadium File`` enter 89757
#. In ``Cal File`` load the ``osiris_041_RES10.cal`` file from the unit test data
#. Select Detector Grouping to be ``All``
#. Click ``Run``
#. There should be three output workspaces, ending in ``_dRange``, ``_q`` and ``_tof``
#. Each of the output workspaces should have 1 spectra
#. Change the Detector Grouping to ``Groups`` and enter 5 groups
#. Click ``Run``
#. Each of the output workspaces should have 5 spectra
#. Change the Detector Grouping to ``Custom``, enter ``3-500,501-962``
#. Click ``Run``
#. Each of the output workspaces should have 2 spectra
