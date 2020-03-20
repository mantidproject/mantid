============
SANS Changes
============

.. contents:: Table of Contents
   :local:

Bug Fixed
#########

- Applying a mask to a range of spectra after cropping to a bank could fail
  if there were gaps in the spectrum numbers. The masking will now skip
  over any spectrum numbers not in workspace and emit a warning.


.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

:ref:`Release 5.1.0 <v5.1.0>`