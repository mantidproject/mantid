.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to save the masking from a workspace to an XML
file. This algorithm has previously been renamed from SaveDetectorMasks.

2 Types of Mask Workspace
-------------------------

There are two types of mask workspace that can serve as input.

1. MaskWorkspace
################

In this case, :ref:`algm-SaveMask` will read Y values to determine
which detectors are masked;

2. A non-MaskWorkspace :ref:`MatrixWorkspace <MatrixWorkspace>` containing :ref:`Instrument <Instrument>`
#########################################################################################################

In this case, :ref:`algm-SaveMask` will scan through all detectors to
determine which are masked.

Definition of Mask
------------------

See :ref:`maskdetectors_def_of_mask`.

XML File Format
---------------

Example 1:

.. code-block:: xml

   <?xml version="1.0" encoding="UTF-8" ?>
   <detector-masking>
     <group>
       <detids>3,34-44,47</detids>
       <component>bank123</component>
       <component>bank124</component>
     </group>
   </detector-masking>

.. categories::

.. sourcelink::
