.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Executes the SANS reduction workflow according to the options set by
`SetupEQSANSReduction <http://www.mantidproject.org/SetupEQSANSReduction>`_ or
`SetupILLD33Reduction <http://www.mantidproject.org/SetupILLD33Reduction>`_.
Those options are saved in a PropertyManager object that is passed through *ReductionProperties*.

The workflow proceeds as follows:

1. Execute the beam finder algorithm. Usually `SANSBeamFinder <http://www.mantidproject.org/SANSBeamFinder>`_.

2. Load the data to be reduced, usually with `EQSANSLoad <http://www.mantidproject.org/EQSANSLoad>`_, 
   which will move the detector to the right position.

3. Subtract the dark current, usually with `EQSANSDarkCurrentSubtraction <http://www.mantidproject.org/EQSANSDarkCurrentSubtraction>`_.

4. Normalize the data, usually with `EQSANSNormalise <http://www.mantidproject.org/EQSANSNormalise>`_.

5. Mask detector pixels as appropriate, usually with `SANSMask <http://www.mantidproject.org/SANSMask>`_.

6. Apply the solid angle correction, usually with `SANSSolidAngleCorrection <http://www.mantidproject.org/SANSSolidAngleCorrection>`_.

7. Apply the sensitivity correction, usually with `SANSSensitivityCorrection <http://www.mantidproject.org/SANSSensitivityCorrection>`_. When applicable, 
   a separate beam center position can be determined for the sensitivity data.
   
8. Compute and apply the transmission correction, usually with `EQSANSDirectBeamTransmission <http://www.mantidproject.org/EQSANSDirectBeamTransmission>`_.
   When applicable, a separate beam center position can be determined for the transmission data.

9. Repeat steps 2 to 8 for the background run, then subtract the background from the sample data.

10. Perform the absolute scaling, usually with `SANSAbsoluteScale <http://www.mantidproject.org/SANSAbsoluteScale>`_.

11. Perform any geometrical correction, which is usually a call to `NormaliseByThickness <http://www.mantidproject.org/NormaliseByThickness>`_.

12. Perform the I(Q) calculation with `EQSANSAzimuthalAverage1D <http://www.mantidproject.org/EQSANSAzimuthalAverage1D>`_.

13. Perform the I(Qx,Qy) calculation with `EQSANSQ2D <http://www.mantidproject.org/EQSANSQ2D>`_.

14. Save the I(Q) output using `SaveAscii <http://www.mantidproject.org/SaveAscii>`_ 
    and using `SaveCanSAS1D <http://www.mantidproject.org/SaveCanSAS1D>`_.

15. Save the I(Qx,Qy) output using `SaveNISTDAT <http://www.mantidproject.org/SaveNISTDAT>`_
    and `SaveNexus <http://www.mantidproject.org/SaveNexus>`_.

.. categories::
