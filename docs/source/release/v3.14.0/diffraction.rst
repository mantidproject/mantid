===================
Diffraction Changes
===================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Powder Diffraction
------------------

Improvements
############

- :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>` will always set the value of
  Phi to 0 when writing MAUD headers. This resolves an issue where data was not
  displayed with a value of -90.0

:ref:`Release 3.14.0 <v3.14.0>`
