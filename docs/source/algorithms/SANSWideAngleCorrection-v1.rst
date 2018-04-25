.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm Reference Discussion
------------------------------

Looking at `Computing guide for Small Angle Scattering
Experiments <https://kur.web.psi.ch/sans1/manuals/sas_manual.pdf>`__ by
Ghosh, Egelhaaf & Rennie, we see that for scattering at larger angles
the transmission should be modified due to the longer path length after
the scattering event.

The longer path length after scattering will also slightly increase the
probability of a second scattering event, but this is not dealt with
here.

If our on-axis transmission is :math:`T_0` through a sample of thickness
:math:`d`, then the transmission at some other thickness :math:`x` is
:math:`\exp(-\mu x)` where attenuation coefficient
:math:`\mu = -\ln( \frac{T_0}{d})`.

If a neutron scatters at angle :math:`2\theta` at distance :math:`x`
into the sample, its total transmission is then:

:math:`T^{''} = \exp(-\mu x) \exp( \frac{-\mu(d-x)}{\cos(2\theta)})`

:math:`T^{''}` should be integrated and averaged between :math:`x = 0`
and :math:`x = d`.

Hammouda, gives an approximate result for the integral, see page 208 of
`SANS toolbox <http://www.ncnr.nist.gov/staff/hammouda/the_SANS_toolbox.pdf>`__:

:math:`T^{'} = \frac{T_0(T_0^A - 1)}{A \ln(T_0)}`

For:

:math:`A = \frac{1}{\cos(2\theta)} - 1`

For example if :math:`T_0 = 0.2` and :math:`2\theta = 40` then
:math:`T^{'} = 0.158`, a shift of :math:`~20`\ % of the SANS curve. Note
that the result is independent of sample thickness.

:math:`T_0` is a function of neutron wavelength, whilst :math:`A` is a
function of detector pixel location.

The output of this algorithm is:

:math:`OutputWorkspace = \frac{T^\prime}{T_0}`

Error Propagation
#################

The error propagation follows this formula:

`` ``\ :math:`OutputWorkspace_{error} = \frac{T_{0E} ^A - 1}{A\ln(T_0E)}`

Which means, that we do not consider the error in the definition of the
:math:`2\theta` (the parameter A)

Enabling Wide Angle Correction for Reduction of SANS ISIS
---------------------------------------------------------

To enable the Wide Angle correction use the User File settings:

``SAMPLE/PATH/ON``

More information on:
`SANS\_User\_File\_Commands#SAMPLE <SANS_User_File_Commands#SAMPLE>`__

SANS ISIS Reduction
-------------------

The output of SANSWideAngleCorrection is used as WavePixelAdj parameter
at :ref:`algm-Q1D`.

Wide Angle Correction and the SANS Reduction
--------------------------------------------

The equation for the reduction is (see :ref:`algm-Q1D`)

:math:`P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}}`

But, :math:`T(n)` is not really :math:`T(n)`, because of the wide
angles, it is now :math:`T(n,theta)` or :math:`T(n,i,j)`.

So, we decided to have a new factor that changes this equation to:

:math:`P_I(Q) = \frac{\sum_{\{i, j, n\} \supset \{I\}} S(i,j,n)}{\sum_{\{i, j, n\} \supset \{I\}}M(n)\eta(n)T(n)\Omega_{i j}F_{i j}Corr(i,j,n)}`

Where Corr (Correction factor) in this case will be:

:math:`Corr = \frac{T_0^A - 1}{A \ln(T_0)}`

Which is the OutputWorkspace of SANSWideAngleCorrection.

This parameter enters inside :ref:`algm-Q1D` as WavePixelAdj. But, this is
all done for you inside the Reduction Script.

Comparison with Wide Angle Correction at SNS
--------------------------------------------

The transmission correction applied at SNS is described
`here <http://www.mantidproject.org/HFIR_SANS#Transmission_correction>`__,
and it is applied through the
:ref:`algm-ApplyTransmissionCorrection` algorithm.
The correction applied there is an approximation for the same equations
described here. The picture above compare their results

.. figure:: /images/SNS_ISIS_WideAngleCorrections.png
   :alt: SNS_ISIS_WideAngleCorrections.png

Note a difference among them is when they are applied. At SNS, the
correction is applied before averaging the counters per bin inside
:ref:`algm-Q1D` algorithm, while at ISIS, it is used after, inside the
:ref:`algm-Q1D` algorithm, for the division of the counters per bin
normalized by the transmission counters.

References
----------

Annie Brulet et al. - Improvement of data treatment in small-angle neutron scattering - `J. Appl.
Cryst. (2007). 40 <http://dx.doi.org/10.1107/S0021889806051442>`_

Ghosh, Egelhaaf & Rennie - Computing guide for Small Angle Scattering
Experiments

Usage
-----

**Example - Correcting Some Dummy Data**

.. testcode:: ExCorrection

   # Create some dummy data, but crop it for quick demonstration purposes.
   sample = CreateSimulationWorkspace(Instrument='SANS2D', BinParams=[5,500,100005], UnitX='TOF')
   sample = CropWorkspace(sample,StartWorkspaceIndex=0,EndWorkspaceIndex=20)

   # Create a dummy transmission workspace.
   transmission = CropWorkspace(sample,StartWorkspaceIndex=10,EndWorkspaceIndex=10)
   transmission *= 2

   corrected_data = SANSWideAngleCorrection(sample, transmission)

   print("{:.6f} was corrected to {:.6f}.".format(sample.readY(19)[0], corrected_data.readY(19)[0]))

Output:

.. testoutput:: ExCorrection

   1.000000 was corrected to 1.004997.

.. categories::

.. sourcelink::
