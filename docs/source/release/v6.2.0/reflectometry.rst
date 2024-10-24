=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

Reflectometry
-------------

Improvements
############
- :ref:`LRSubtractAverageBackground <algm-LRSubtractAverageBackground-v1>` now optionally uses weighted error for background with new input property ErrorWeighting.
- :ref:`LiquidsReflectometryReduction <algm-LiquidsReflectometryReduction-v1>` now optionally uses weighted error for background with new input property ErrorWeighting with the default as False.
- Version 2 of :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` has been added which assumes detectors are at :math:`2\theta` rather than :math:`\theta_f`. Use version 1 if you require detectors at :math:`\theta_f`.
- The detectors positions in the INTER IDF were changed from theta to two theta.

:ref:`Release 6.2.0 <v6.2.0>`
