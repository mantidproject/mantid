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

- Changing settings while running methods on the PEARL object no
  longer updates the default settings. Instead, initial settings are
  taken as the default, and any changes are reverted back to the
  default once the line they were made on has finished executing
    
:ref:`Release 3.13.0 <v3.13.0>`
