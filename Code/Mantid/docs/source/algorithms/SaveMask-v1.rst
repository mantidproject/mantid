.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to save the masking from a workspace to an XML
file. This algorithm has previously been renamed from SaveDetectorMasks.

2 Types of Mask Workspace
-------------------------

There are two types of mask workspace that can serve as input.

1. `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__
##################################################################

In this case, :ref:`algm-SaveMask` will read Y values to determine
which detectors are masked;

2. A non-\ `MaskWorkspace <http://www.mantidproject.org/MaskWorkspace>`__ :ref:`MatrixWorkspace <MatrixWorkspace>` containing :ref:`Instrument <Instrument>`
################################################################################################################################################################################################################

In this case, :ref:`algm-SaveMask` will scan through all detectors to
determine which are masked.

Definition of Mask
------------------

If a pixel is **masked**, it means that the data from this pixel won't be used.
In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`__), the corresponding value is 1. 

If a pixel is **NOT masked**, it means that the data from this pixel will be used.  ``
In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`__), the corresponding value is 0.

XML File Format
---------------

Example 1:

.. code-block:: xml

   <?xml version="1.0" encoding="UTF-8" ?>
   <detector-masking">
     <group">
       <detids>3,34-44,47</detids>
       <component>bank123</component>
       <component>bank124</component>
     </group>
   </detector-masking>

.. categories::
