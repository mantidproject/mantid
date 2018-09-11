.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Worfklow algorithm used to compute and apply the sample transmission correction using 
the beam spreader ("glassy carbon") method. The transmission is calculated by the 
:ref:`CalculateTransmission <algm-CalculateTransmission>` as follows:

:math:`T=\frac{N_{gc,\ sample}/T_{gc,\ sample} - T_{gc}N_{sample}/T_{sample}}{N_{gc,\ empty}/T_{gc,\ empty} - T_{gc}N_{empty}/T_{empty}}`

where :math:`N_{gc}`, sample and :math:`N_{gc}`, empty are the sums of all pixel counts 
for the sample and direct beam data sets with glass carbon, and 
:math:`N_{sample}` and :math:`N_{empty}` are the sums 
of all the pixel counts for the sample and direct beam without glassy carbon. 
The T values are the corresponding counting times. If the user chose to normalize the 
data using the beam monitor when setting up the reduction process, the beam monitor 
will be used to normalize all data sets instead of the timer.
If the user chose to use a dark current data set when starting the reduction process, 
that dark current data will be subtracted from all data sets before the transmission 
is calculated.

For each detector pixel, the transmission correction is applied by the 
:ref:`ApplyTransmissionCorrection <algm-ApplyTransmissionCorrection>` as follows:

:math:`I'(x,y)=\frac{I(x,y)}{T^{[1+\sec(2\theta)]/2}}
\sigma_{I'(x,y)}=[[{\frac{\sigma_I}{{T^{[1+\sec(2\theta)]/2}}}}]^2 + [{\frac{I(x,y)\sigma_T(\frac{1+\sec(2\theta)}{2})}{{T^{[\sec(2\theta)-1]/2}}}}]^2]^{1/2}`

This algorithm is rarely called directly. It is called by 
:ref:`HFIRSANSReduction <algm-HFIRSANSReduction>`.

.. categories::

.. sourcelink::
