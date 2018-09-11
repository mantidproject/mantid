.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Executes the HFIR SANS reduction workflow according to the options set by 
:ref:`SetupHFIRReduction <algm-SetupHFIRReduction>`.
Those options are saved in a PropertyManager object that is passed through *ReductionProperties*.

The workflow proceeds as follows:

1. Execute the beam finder algorithm. Usually :ref:`SANSBeamFinder <algm-SANSBeamFinder>`.

2. Load the data to be reduced, usually with :ref:`HFIRLoad <algm-HFIRLoad>`, 
   which will move the detector to the right position.

3. Subtract the dark current, usually with :ref:`HFIRDarkCurrentSubtraction <algm-HFIRDarkCurrentSubtraction>`.

4. Normalize the data, usually with :ref:`HFIRSANSNormalise <algm-HFIRSANSNormalise>`.

5. Mask detector pixels as appropriate, usually with :ref:`SANSMask <algm-SANSMask>`.

6. Apply the solid angle correction, usually with :ref:`SANSSolidAngleCorrection <algm-SANSSolidAngleCorrection>`.

7. Apply the sensitivity correction, usually with :ref:`SANSSensitivityCorrection <algm-SANSSensitivityCorrection>`. 
   When applicable, a separate beam center position can be determined for the sensitivity data.
   
8. Compute and apply the transmission correction, usually with :ref:`SANSDirectBeamTransmission <algm-SANSDirectBeamTransmission>`. 
   When applicable, a separate beam center position can be determined for the transmission data.

9. Repeat steps 2 to 8 for the background run, then subtract the background from the sample data.

10. Perform the absolute scaling, usually with :ref:`SANSAbsoluteScale <algm-SANSAbsoluteScale>`. 

11. Perform any geometrical correction, which is usually a call to :ref:`NormaliseByThickness <algm-NormaliseByThickness>`. 

12. Perform the I(Q) calculation with :ref:`SANSAzimuthalAverage1D <algm-SANSAzimuthalAverage1D>`. 

13. Perform the I(Qx,Qy) calculation with :ref:`EQSANSQ2D <algm-EQSANSQ2D>` 
    (This is not a typo. This algorithm is used for both HFIR SANS and EQSANS).

14. Save the I(Q) output using :ref:`SaveAscii <algm-SaveAscii>`
    and using :ref:`SaveCanSAS1D <algm-SaveCanSAS1D>`.

15. Save the I(Qx,Qy) output using :ref:`SaveNISTDAT <algm-SaveNISTDAT>`.

|HFIRSANSReduction.png|

.. |HFIRSANSReduction.png| image:: /images/HFIRSANSReduction.png

.. categories::

.. sourcelink::
