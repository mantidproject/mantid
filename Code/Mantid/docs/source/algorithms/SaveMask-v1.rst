.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to save the masking from a workspace to an XML
file. This algorithm has previously been renamed from
`SaveDetectorMasks <SaveDetectorMasks>`__.

2 Types of Mask Workspace
-------------------------

There are two types of mask workspace that can serve as input.

1. `MaskWorkspace <MaskWorkspace>`__
####################################

In this case, :ref:`algm-SaveMask` will read Y values to determine
which detectors are masked;

2. A non-\ `MaskWorkspace <MaskWorkspace>`__ `MatrixWorkspace <MatrixWorkspace>`__ containing `Instrument <Instrument>`__
#########################################################################################################################

In this case, :ref:`algm-SaveMask` will scan through all detectors to
determine which are masked.

Definition of Mask
------------------

| ``* If a pixel is masked, it means that the data from this pixel won't be used.  ``
| ``  In the masking workspace (i.e., ``\ ```SpecialWorkspace2D`` <SpecialWorkspace2D>`__\ ``), the corresponding value is 1. ``
| ``* If a pixel is NOT masked, it means that the data from this pixel will be used.  ``
| ``  In the masking workspace (i.e., ``\ ```SpecialWorkspace2D`` <SpecialWorkspace2D>`__\ ``), the corresponding value is 0.``

XML File Format
---------------

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

.. categories::
