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

* If a pixel is masked, it means that the data from this pixel won't be used. In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`_ ), the corresponding value is 1.
* If a pixel is NOT masked, it means that the data from this pixel will be used. In the masking workspace (i.e., `SpecialWorkspace2D <http://www.mantidproject.org/SpecialWorkspace2D>`_ ), the corresponding value is 0.

File Format
-----------

XML File Format
###############

Example 1::

 <?xml version="1.0" encoding="UTF-8" ?>
 <detector-masking>
  <group>
   <detids>3,34-44,47</detids>
   <component>bank123</component>
   <component>bank124</component>
  </group>
 </detector-masking>

ISIS File Format
################

Example 2::

 1-3 62-64
 65-67 126-128
 129-131 190-192
 193-195 254-256
 257-259 318-320
 321-323 382-384
 385 387 446 448
 ... ...

All the integers in file of this format are spectrum Numbers to mask. Two
spectrum Numbers with "-" in between indicate a continuous range of spectra
to mask. It does not matter if there is any space between integer number
and "-". There is no restriction on how the line is structured. Be
noticed that any line starting with a non-digit character, except space,
will be treated as a comment line.

This algorithm loads masking file to a SpecialWorkspace2D/MaskWorkspace.

Supporting ::

 * Component ID --> Detector IDs --> Workspace Indexes
 * Detector ID --> Workspace Indexes
 * Spectrum Number --> Workspace Indexes

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: DoIt

    ws = Load('HYS_11092_event.nxs')
    mask = LoadMask('HYS', 'HYS_mask.xml')
    # To check if mask loaded, apply it
    MaskDetectors(ws, MaskedWorkspace=mask)
    # One can alternatively do
    # ws.maskDetectors(MaskedWorkspace=mask)

    # Check some pixels
    print "Is detector 0 masked:", ws.getDetector(0).isMasked()
    print "Is detector 6245 masked:", ws.getDetector(6245).isMasked()
    print "Is detector 11464 masked:", ws.getDetector(11464).isMasked()
    print "Is detector 17578 masked:", ws.getDetector(17578).isMasked()
    print "Is detector 20475 masked:", ws.getDetector(20475).isMasked()

Output:

.. testoutput:: DoIt

    Is detector 0 masked: True
    Is detector 6245 masked: True
    Is detector 11464 masked: False
    Is detector 17578 masked: False
    Is detector 20475 masked: True

.. categories::

.. sourcelink::
