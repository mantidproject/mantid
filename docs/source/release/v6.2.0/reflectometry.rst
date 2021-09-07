=====================
Reflectometry Changes
=====================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Reflectometry
-------------

Improvements
############
- :ref:`LRSubtractAverageBackground <algm-LRSubtractAverageBackground-v1>` now optionally uses weighted error for background with new input property ErrorWeighting.
- :ref:`LiquidsReflectometryReduction <algm-LiquidsReflectometryReduction-v1>` now optionally uses weighted error for background with new input property ErrorWeighting with the default as False.

- Version 2 of :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>` has been added which assumes detectors are at :math:`2\theta` rather than :math:`\theta_f`. Use version 1 if you require detectors at :math:`\theta_f`.

:ref:`Release 6.2.0 <v6.2.0>`

