.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Executes the SANS reduction workflow according to the options set by 
:ref:`SetupEQSANSReduction <algm-SetupEQSANSReduction>` or :ref:`SetupILLD33Reduction <algm-SetupILLD33Reduction>`.
Those options are saved in a PropertyManager object that is passed through *ReductionProperties*.

The workflow proceeds as follows:

1. Execute the beam finder algorithm. Usually :ref:`SANSBeamFinder <algm-SANSBeamFinder>`.

2. Load the data to be reduced, usually with :ref:`EQSANSLoad <algm-EQSANSLoad>`, 
   which will move the detector to the right position.

3. Subtract the dark current, usually with :ref:`EQSANSDarkCurrentSubtraction <algm-EQSANSDarkCurrentSubtraction>`.

4. Normalize the data, usually with :ref:`EQSANSNormalise <algm-EQSANSNormalise>`.

5. Mask detector pixels as appropriate, usually with :ref:`SANSMask <algm-SANSMask>`.

6. Apply the solid angle correction, usually with :ref:`SANSSolidAngleCorrection <algm-SANSSolidAngleCorrection>`.

7. Apply the sensitivity correction, usually with :ref:`SANSSensitivityCorrection <algm-SANSSensitivityCorrection>`. 
   When applicable, a separate beam center position can be determined for the sensitivity data.
   
8. Compute and apply the transmission correction, usually with :ref:`EQSANSDirectBeamTransmission <algm-EQSANSDirectBeamTransmission>`.
   When applicable, a separate beam center position can be determined for the transmission data.

9. Repeat steps 2 to 8 for the background run, then subtract the background from the sample data.

10. Perform the absolute scaling, usually with :ref:`SANSAbsoluteScale <algm-SANSAbsoluteScale>`. 

11. Perform any geometrical correction, which is usually a call to :ref:`NormaliseByThickness <algm-NormaliseByThickness>`. 

12. Perform the I(Q) calculation with :ref:`EQSANSAzimuthalAverage1D <algm-EQSANSAzimuthalAverage1D>`. 

13. Perform the I(Qx,Qy) calculation with :ref:`EQSANSQ2D <algm-EQSANSQ2D>`.

14. Save the I(Q) output using :ref:`SaveAscii <algm-SaveAscii>`
    and using :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>`.

15. Save the I(Qx,Qy) output using :ref:`SaveNISTDAT <algm-SaveNISTDAT>`
    and :ref:`SaveNexus <algm-SaveNexus>`.

|SANSReduction.png|

.. |SANSReduction.png| image:: /images/SANSReduction.png

.. categories::

.. sourcelink::
