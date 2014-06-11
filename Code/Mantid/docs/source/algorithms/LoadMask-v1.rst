.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to load a masking file, which can be in XML
format (defined later in this page) or old-styled calibration file.

Definition of Mask
------------------

| ``* If a pixel is masked, it means that the data from this pixel won't be used.  ``
| ``  In the masking workspace (i.e., ``\ ```SpecialWorkspace2D`` <SpecialWorkspace2D>`__\ ``), the corresponding value is 1. ``
| ``* If a pixel is NOT masked, it means that the data from this pixel will be used.  ``
| ``  In the masking workspace (i.e., ``\ ```SpecialWorkspace2D`` <SpecialWorkspace2D>`__\ ``), the corresponding value is 0.``

File Format
-----------

XML File Format
###############

Example 1:

.. raw:: html

   <?xml version="1.0" encoding="UTF-8" ?>

| `` ``\ 
| ``  ``\ 
| ``   ``\ \ ``3,34-44,47``\ 
| ``   ``\ \ ``bank123``\ 
| ``   ``\ \ ``bank124``\ 
| ``  ``\ 
| `` ``\ 

ISIS File Format
################

Example 2:

| `` 1-3 62-64``
| `` 65-67 126-128``
| `` 129-131 190-192``
| `` 193-195 254-256``
| `` 257-259 318-320``
| `` 321-323 382-384``
| `` 385 387 446 448``
| `` ... ...``

All the integers in file of this format are spectrum IDs to mask. Two
spectrum IDs with "-" in between indicate a continuous range of spectra
to mask. It does not matter if there is any space between integer number
and "-". There is no restriction on how the line is structured. Be
noticed that any line starting with a non-digit character, except space,
will be treated as a comment line.

This algorithm loads masking file to a SpecialWorkspace2D/MaskWorkspace.

Supporting

| ``* Component ID --> Detector IDs --> Workspace Indexes``
| ``* Detector ID --> Workspace Indexes``
| ``* Spectrum ID --> Workspace Indexes``

.. categories::
