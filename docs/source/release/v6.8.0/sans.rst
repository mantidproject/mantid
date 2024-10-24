============
SANS Changes
============

.. contents:: Table of Contents
   :local:

New Features
------------
- ``Background Workspace`` and ``Scale Factor`` can now be given on the ISIS SANS Interface by checking the ``Scaled Background Subtraction`` checkbox.
  See :ref:`ISIS_SANS_scaled_background-ref` for more details.
- Algorithms :ref:`algm-SANSTubeCalibration` and :ref:`algm-SANSTubeMerge` have been added for calibrating the Sans2d instrument at ISIS.

Bugfixes
--------
- Additional parameter ``IntegrationRadius`` in :ref:`FindCenterOfMassPosition <algm-FindCenterOfMassPosition>` allows reasonable results for asymmetric detector arrays (e.g. BioSANS with midrange and wing detectors)

:ref:`Release 6.8.0 <v6.8.0>`
