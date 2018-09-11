.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Worfklow algorithm used to compute and apply the sample transmission correction using 
the direct beam method. The transmission is calculated by the
:ref:`CalculateTransmission <algm-CalculateTransmission>`
as follows:

:math:`T=\frac{\sum_{i;\ d(i,j)<R}\ \sum_{j} \frac{I_{sample}(i,j)}{T_{sample}}}{\sum_{i;\ d(i,j)<R}\ \sum_{j} \frac{I_{beam}(i,j)}{T_{beam}}}`

where :math:`I_{sample}` and :math:`I_{beam}` are the pixel counts for the sample 
data set and the direct beam data set, respectively. The sums for each data set runs 
only over the pixels within a distance R of the beam center equal to the beam radius. 
:math:`T_{sample}` and :math:`T_{beam}` are the counting times for each of the 
two data sets. If the user chose to normalize the data using the beam monitor when 
setting up the reduction process, the beam monitor will be used to normalize the 
sample and direct beam data sets instead of the timer.
If the user chose to use a dark current data set when starting the reduction process, 
that dark current data will be subtracted from both data sets before the transmission 
is calculated.


For each detector pixel, the transmission correction is applied by the
:ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>`
as follows:

:math:`I'(x,y)=\frac{I(x,y)}{T^{[1+\sec(2\theta)]/2}}
\sigma_{I'(x,y)}=[[{\frac{\sigma_I}{{T^{[1+\sec(2\theta)]/2}}}}]^2 + [{\frac{I(x,y)\sigma_T(\frac{1+\sec(2\theta)}{2})}{{T^{[\sec(2\theta)-1]/2}}}}]^2]^{1/2}`

This algorithm is rarely called directly. It is called by 
:ref:`HFIRSANSReduction <algm-HFIRSANSReduction>` or :ref:`EQSANSDirectBeamTransmission <algm-EQSANSDirectBeamTransmission>`.

.. categories::

.. sourcelink::
