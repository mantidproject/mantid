.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates a Multiple Scattering correction using a Monte Carlo integration method.
The method uses a structure function for the sample to determine the probability of a particular momentum transfer (q) and energy transfer (:math:`\omega`) value for each scattering event and it doesn't therefore rely on an assumption that the scattering is isotropic.

The structure function that the algorithm takes as input is a linear combination of the coherent and incoherent structure factors:

:math:`S'(Q) = \frac{1}{\sigma_b}(\sigma_{coh} S(Q, \omega) + \sigma_{inc} S_s(Q, \omega))`

If the sample is a perfectly coherent scatterer then :math:`S'(Q, \omega) = S(Q, \omega)`

The algorithm is based on code which was originally written in Fortran as part of the Discus program [#JOH]_. The code was subsequently resurrected and improved by Spencer Howells under the Muscat name and was included in the QENS MODES package [#HOW]_
These original programs calculated multiple scattering corrections for inelastic instruments but an elastic diffraction version of the code was also created and results from that program are included in this paper by Mancinelli [#MAN]_.

Theory
######

The theory is outlined here for an inelastic calculation. The calculation performed for an elastic instrument is a special case of this with :math:`\omega=0`.

The algorithm calculates a set of dimensionless weights :math:`J_n` describing the probability of detection at an angle :math:`\theta` after n scattering events given a total incident flux :math:`I_0` and a transmitted flux of T:

:math:`T_n(\theta,\lambda) = J_n I_0(\lambda)`

The quantity :math:`J_n` is calculated by performing the following integration:

.. math::

   J_n &= (\frac{\mu_s}{4 \pi})^n \frac{1}{A} \int dS \int_{0}^{l_1^{max}} dl_1 e^{-\mu_T l_1} \prod\limits_{i=1}^{n-1} [\int_{0}^{l_{i+1}^{max}} dl_{i+1} \int_{0}^{\pi} \sin\theta_i d\theta_i \int_{0}^{2 \pi} d\phi_i (e^{-\mu_T l_{i+1}}) S(Q_i)] e^{-\mu_T l_{out}} S(Q_n) \\
       &=(\frac{\mu_s}{4 \pi})^n \frac{1}{A} \int dS \int_{0}^{l_1^{max}} dl_1 e^{-\mu_T l_1} \prod\limits_{i=1}^{n-1} [\int_{0}^{l_{i+1}^{max}} dl_{i+1} \int_{0}^{2k} dQ_i \int_{0}^{2 \pi} d\phi_i (e^{-\mu_T l_{i+1}}) \frac{Q_i}{k^2} S(Q_i)] e^{-\mu_T l_{out}} S(Q_n)


The variables :math:`l_i^{max}` represent the maximum path length before the next scatter given a particular phi and theta value (Q). Each :math:`l_i` is actually a function of all of the earlier values for the :math:`l_i`, :math:`\phi` and :math:`Q` variables ie :math:`l_i = l_i(l_1, l_2, ..., l_{i-1}, \phi_1, \phi_2, ..., \phi_i, Q_1, Q_2, ..., Q_i)`

The following substitutions are then performed in order to make it more convenient to evaluate as a Monte Carlo integral:

:math:`t_i = \frac{1-e^{-\mu_T l_i}}{1-e^{-\mu_T l_i^{max}}}`

:math:`u_i = \frac{\phi_i}{2 \pi}`

:math:`2 k^2 = \frac{\sigma_S \int_0^{2k} Q_i S(Q_i) dQ}{\sigma_s(k)}`

Using the new variables the integral is:

.. math::

   J_n = \frac{1}{A} \int dS \int_{0}^{1} dt_1 \frac{1-e^{-\mu_T l_1^{\ max}}}{\sigma_T} \prod\limits_{i=1}^{n-1}[\int_{0}^{1} dt_{i+1} \int_{0}^{2k} dQ_i \int_{0}^{1} du_i \frac{(1-e^{-\mu_T l_{i+1}^{max}})}{\sigma_T} \frac{Q_i}{\int_0^{2k} Q_i S(Q_i) dQ_i} S(Q_i) \sigma_s(k)] e^{-\mu_T l_{out}} S(Q_n) \frac{\sigma_s}{4 \pi}

This is evaluated as a Monte Carlo integration by selecting random values for the variables :math:`t_i` and :math:`u_i` between 0 and 1 and values for :math:`Q_i` between 0 and 2k.
A simulated path is traced through the sample to enable the :math:`l_i^{\ max}` values to be calculated. The path is traced by calculating the :math:`l_i`, :math:`\theta` and :math:`\phi` values as follows from the random variables. The code keeps a note of the start coordinates of the current path segment and updates it when moving to the next segment using these randomly selected lengths and directions:

:math:`l_i = -\frac{1}{\mu_T}ln(1-(1-e^{-\mu_T l_i^{\ max}})t_i)`

:math:`\cos\theta_i = 1 - Q_i^2/k^2`

:math:`\phi_i = 2 \pi u_i`

The final Monte Carlo integration that is actually performed by the code is as follows where N is the number of scenarios:

.. math::

   J_n = \frac{1}{N}\sum \frac{1-e^{-\mu_T l_1^{\ max}}}{\sigma_T} \prod\limits_{i=1}^{n-1}[\frac{(1-e^{-\mu_T l_{i+1}^{max}})}{\sigma_T} \frac{Q_i}{<Q S(Q)>} S(Q_i) \sigma_s(k)] e^{-\mu_T l_{out}} S(Q_n) \frac{\sigma_s}{4 \pi}

The purpose of replacing :math:`2 k^2` with :math:`\int Q S(Q) dQ` can now be seen because it avoids the need to multiply by an integration range across :math:`dQ` when converting the integral to a Monte Carlo integration.
This is useful in the inelastic version of this algorithm where the integration of the structure factor is over two dimensions :math:`Q` and :math:`\omega` and the area of :math:`Q\omega` space that has been integrated over is less obvious.

This is similar to the formulation described in the Mancinelli paper except there is no random variable to decide whether a particular scattering event is coherent or incoherent.

Importance Sampling
^^^^^^^^^^^^^^^^^^^

The algorithm includes an option to use importance sampling to improve the results for S(Q) profiles containing spikes.
Without this option enabled, the contribution from rare, high values in the structure factor is only visible at a very high number of scenarios.

The importance sampling is achieved using a further change of variables as follows:

:math:`v_i = P(Q_i) = \frac{I(Q_i)}{I(2k)}` where :math:`I(x) = \int_{0}^{x} Q S(Q) dQ`

With this approach the Q value for each segment is chosen as follows based on a :math:`v_i` value randomly selected between 0 and 1:

:math:`Q_i = P^{-1}(v_i)`

:math:`\cos\theta_i` is determined from :math:`Q_i` as before. The change of variables gives the following integral for :math:`J_n`:

.. math::

   J_n = \frac{1}{A} \int dS \int_{0}^{1} dt_1 \frac{1-e^{-\mu_T l_1^{\ max}}}{\sigma_T} \prod\limits_{i=1}^{n-1}[\int_{0}^{1} dt_{i+1} \int_{0}^{1} dv_i 2 \frac{I(2k)}{2k^2} \sigma_s \int_{0}^{1} du_i \frac{(1-e^{-\mu_T l_{i+1}^{max}})}{\sigma_T}] e^{-\mu_T l_{out}} S(Q_n) \frac{\sigma_s}{4 \pi}

   J_n = \frac{1}{A} \int dS \int_{0}^{1} dt_1 \frac{1-e^{-\mu_T l_1^{\ max}}}{\sigma_T} \prod\limits_{i=1}^{n-1}[\int_{0}^{1} dt_{i+1} \int_{0}^{1} dv_i 2 \sigma_s(k) \int_{0}^{1} du_i \frac{(1-e^{-\mu_T l_{i+1}^{max}})}{\sigma_T}] e^{-\mu_T l_{out}} S(Q_n) \frac{\sigma_s}{4 \pi}

Finally, the equivalent Monte Carlo integration that the algorithm performs with importance sampling enabled is:

.. math::

   J_n = \frac{1}{N}\sum \frac{1-e^{-\mu_T l_1^{\ max}}}{\sigma_T} \prod\limits_{i=1}^{n-1}[2 \sigma_s(k) \frac{(1-e^{-\mu_T l_{i+1}^{max}})}{\sigma_T}] e^{-\mu_T l_{out}} S(Q_n) \frac{\sigma_s}{4 \pi}


Outputs
#######

The algorithm outputs a workspace group containing the following workspaces:

- Several workspaces called Scatter_n where n is the number of scattering events considered. Each workspace contains "per detector" weights as a function of wavelength for a specific number of scattering events. The number of scattering events ranges between 1 and the number specified in the NumberOfScatterings parameter
- A workspace called Scatter_1_NoAbsorb is also created for a scenario where neutrons are scattered once, absorption is assumed to be zero and re-scattering after the simulated scattering event is assumed to be zero. This is the quantity :math:`J_{1}^{*}` described in the Discus manual
- A workspace called Scatter_2_n_Summed which is the sum of the Scatter_n workspaces for n > 1

The output can be applied to a workspace containing a real sample measurement in one of two ways:

- subtraction method. The additional intensity contributed by multiple scattering to either a raw measurement or a vanadium corrected measurement can be calculated from the weights output from this algorithm. The additional intensity can then be subtracted to give an idealised "single scatter" intensity.
  For example, the additional intensity measured at a detector due to multiple scattering is given by :math:`(\sum_{n=2}^{\infty} J_n) E(\lambda) I_0(\lambda) \Delta \Omega` where :math:`E(\lambda)` is the detector efficiency, :math:`I_0(\lambda)` is the incident intensity and :math:`\Delta \Omega` is the solid angle subtended by the detector.
  The factors :math:`E(\lambda) I_0(\lambda) \Delta \Omega` can be obtained from a Vanadium run - although to take advantage of the "per detector" multiple scattering weights, the preparation of the Vanadium data will need to take place "per detector" instead of on focussed datasets
- factor method. The correction can be applied by multiplying the real sample measurement by :math:`J_1/\sum_{n=1}^{\infty} J_n`. This approach avoids having to create a suitably normalised intensity from the weights and the method is also more tolerant of any normalisation inaccuracies in the S(Q) profile

The multiple scattering correction should be applied before applying an absorption correction.

The Discus manual describes a further method of applying an attenuation correction and a multiple scattering correction in one step using a variation of the factor method. To achieve this the real sample measurement should be multipled by :math:`J_1^{*}/(\sum_{n=1}^{\infty} J_n`).
Note that this differs from the approach taken in other Mantid absorption correction algorithms such as MonteCarloAbsorption because of the properties of :math:`J_{1}^{*}`.
:math:`J_{1}^{*}` corrects for attenuation due to absorption before and after the simulated scattering event (which is the same as MonteCarloAbsorption) but it only corrects for attenuation due to scattering after the simulated scattering event.
For this reason it's not clear this feature from Discus is useful but it has been left in for historical reasons.

The sample shape can be specified by running the algorithms :ref:`SetSample <algm-SetSample>` or :ref:`LoadSampleShape <algm-LoadSampleShape>` on the input workspace prior to running this algorithm.

The algorithm can take a long time to run on instruments with a lot of spectra and\or a lot of bins in each spectrum. The run time can be reduced by enabling the following interpolation features:

- the multiple scattering correction can be calculated on a subset of the wavelength bins in the input workspace by specifying a non-default value for NumberOfWavelengthPoints. The other wavelength points will be calculated by interpolation
- the algorithm can be performed on a subset of the detectors by setting SparseInstrument=True

Both of these interpolation features are described further in the documentation for the :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` algorithm

Usage
-----

**Example - elastic calculation on single spike S(Q)**

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid import mtd
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np

   # S(Q) consisting of single spike at q=1
   X=[0.99,1.0,1.01]
   Y=[0.,1000,0.]
   Sofq=CreateWorkspace(DataX=X,DataY=Y,UnitX="MomentumTransfer")

   two_thetas=[]
   for i in range(180):
       two_thetas.append(i)

   # workspace with single bin centred at k=1 Angstrom-1
   ws = CreateSampleWorkspace(WorkspaceType="Histogram",
                              XUnit="Momentum",
                              Xmin=0.5,
                              Xmax=1.5,
                              BinWidth=1.0,
                              NumBanks=len(two_thetas)//4,
                              BankPixelWidth=2,
                              InstrumentName="testinst")

   ids = list(range(1,len(two_thetas)+1))
   EditInstrumentGeometry(ws,
       PrimaryFlightPath=14.0,
       SpectrumIDs=ids,
       L2=[2.0] * len(two_thetas),
       Polar=two_thetas,
       Azimuthal=[90.0] * len(two_thetas),
       DetectorIDs=ids,
       InstrumentName="testinst")

   sphere_xml = " \
   <sphere id='some-sphere'> \
       <centre x='0.0'  y='0.0' z='0.0' /> \
       <radius val='0.01' /> \
   </sphere> \
   <algebra val='some-sphere' /> \
   "
   SetSample(InputWorkspace=ws,
             Geometry={'Shape': 'CSG', 'Value': sphere_xml},
             Material={'NumberDensity': 0.02, 'AttenuationXSection': 0.0,
                       'CoherentXSection': 0.0, 'IncoherentXSection': 0.0, 'ScatteringXSection': 80.0})

   DiscusMultipleScatteringCorrection(InputWorkspace=ws, StructureFactorWorkspace=Sofq,
                                      OutputWorkspace="MuscatResults", NeutronPathsSingle=1000,
                                      NeutronPathsMultiple=10000, ImportanceSampling=True)

   # q=2ksin(theta), so q spike corresonds to single scatter spike at ~60 degrees, double scatter spikes at 0 and 120 degrees
   msplot = plotBin('Scatter_2',0)
   msplot = plotBin('Scatter_1',0, window=msplot)
   axes = plt.gca()
   axes.set_xlabel('Spectrum (~scattering angle in degrees)')
   plt.title("Single and Double Scatter Intensities")
   mtd.clear()

**Example - inelastic calculation on direct geometry (matches calculation in DISCUS paper** [#JOH]_ **figure 1)**

.. plot::
   :include-source:

   # import mantid algorithms, numpy and matplotlib
   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   import math

   # parameterised Lorentzian S(Q,w) from Discus pdf
   # wavelength = 4 Angstroms, k=1.57
   X,Y, SpecAxis =[],[],[]
   qmin, qmax = 0.,4.0
   nqpts = 9
   wmin, wmax = -5.85, 5.85 # meV
   nwpts = 79 # negative w is given explicitly so ~double number of pts in Discus
   D = 0.15 # Angstom-2 meV -1 = 2.3E-05 cm2 s-1
   TEMP=300
   HOVERT = 11.6087/TEMP
   for iq in range(nqpts):
      q = iq * (qmax-qmin)/(nqpts-1) + qmin
      SpecAxis.append(q)
      for iw in range(nwpts):
        w = iw * (wmax-wmin)/(nwpts-1) + wmin
        X.append(w)
        if (w*w + (D*q*q)**2==0.):
           # Discus S(Q,w) has zero here so do likewise
           print("Denominator zero so outputting S(q,w)=0")
           Y.append(0.)
        else:
           Sqw = D*q*q/(math.pi*(w*w + (D*q*q)**2))
           # Apply detailed balance, neutrons more likely to lose energy in each scatter
           # Mantid has w = Ei-Ef
           if (w > 0.):
              Sqw = Sqw * math.exp(HOVERT * w)
           # S(Q,w) is capped at exactly 4.0 for some reason in Discus example
           Y.append(min(Sqw,4.0))

   sqw = CreateWorkspace(DataX=X,DataY=Y,UnitX="DeltaE",
                         VerticalAxisUnit="MomentumTransfer",
                         VerticalAxisValues=SpecAxis, NSpec=nqpts)

   two_thetas = [20.0, 40.0, 60.0, 90.0]

   ws = CreateSampleWorkspace(WorkspaceType="Histogram",
                              XUnit="DeltaE",
                              Xmin=wmin-0.5*(wmax-wmin)/(nwpts-1),
                              Xmax=wmax+0.5*(wmax-wmin)/(nwpts-1),
                              BinWidth=(wmax-wmin)/(nwpts-1),
                              NumBanks=len(two_thetas),
                              BankPixelWidth=1,
                              InstrumentName="testinst")

   # set up ring of detectors in yz plane
   ids = list(range(1,len(two_thetas)+1))
   EditInstrumentGeometry(ws,
       PrimaryFlightPath=14.0,
       SpectrumIDs=ids,
       L2=[2.0] * len(two_thetas),
       Polar=two_thetas,
       Azimuthal=[90.0] * len(two_thetas),
       DetectorIDs=ids,
       InstrumentName="testinst")

   # flat plate sample 5cm x 5cm x 0.065cm
   cuboid_xml = " \
   <cuboid id='flatplate'> \
     <width val='0.05' /> \
     <height val='0.05'  /> \
     <depth  val='0.00065' /> \
     <centre x='0.0' y='0.0' z='0.0'  /> \
     <rotate x='45' y='0' z='0' /> \
   </cuboid> \
   "
   SetSample(InputWorkspace=ws,
             Geometry={'Shape': 'CSG', 'Value': cuboid_xml},
             Material={'NumberDensity': 0.02, 'AttenuationXSection': 0.0,
                       'CoherentXSection': 0.0, 'IncoherentXSection': 0.0, 'ScatteringXSection': 80.0})

   #match Ei value from DISCUS pdf Figure 1
   ws.run().addProperty("deltaE-mode", "Direct", True);
   ws.run().addProperty("Ei", 5.1, True);

   DiscusMultipleScatteringCorrection(InputWorkspace=ws, StructureFactorWorkspace=sqw,
                                      OutputWorkspace="MuscatResults", NeutronPathsSingle=200,
                                      NeutronPathsMultiple=1000)

   # reverse w axis because Discus w = Ef-Ei (opposite to Mantid)
   for i in range(mtd['Scatter_1'].getNumberHistograms()):
       y = np.flip(mtd['Scatter_1'].dataY(i),0)
       mtd['Scatter_1'].setY(i,y.tolist())
   for i in range(mtd['Scatter_2'].getNumberHistograms()):
       y = np.flip(mtd['Scatter_2'].dataY(i),0)
       mtd['Scatter_2'].setY(i,y.tolist())

   plt.rcParams['figure.figsize'] = (5, 6)
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   for i, tt in enumerate(two_thetas):
       ax.plot(mtd['Scatter_1'], wkspIndex=i, label='Single: ' + str(tt) + ' degrees')
   for i, tt in enumerate(two_thetas):
       ax.plot(mtd['Scatter_2'], wkspIndex=i, label='Double: ' + str(tt) + ' degrees', linestyle='--')
   plt.yscale('log')
   ax.set_xlim(-1,1)
   ax.set_ylim(1e-4,1e-1)
   ax.legend(fontsize=7.0)
   plt.title("Inelastic Double\\Single Scattering Weights")
   fig.show()
   mtd.clear()

This is the equivalent plot from the original Discus Fortran program:

.. figure:: /images/DiscusMultipleScatteringFigure1.png


References
##########

.. [#JOH] M W Johnson, 1974 AERE Report R7682, Discus: A computer program for the calculating of multiple scattering effects in inelastic neutron scattering experiments
.. [#HOW] WS Howells, V Garcia Sakai, F Demmel, MTF Telling, F Fernandez-Alonso, Feb 2010, MODES manual RAL-TR-2010-006, `doi: 10.5286/raltr.2010006 <https://doi.org/10.5286/raltr.2010006>`_
.. [#MAN] R Mancinelli 2012 *J. Phys.: Conf. Ser.* **340** 012033, Multiple neutron scattering corrections. Some general equations to do fast evaluations `doi: 10.1088/1742-6596/340/1/012033 <https://doi.org/10.1088/1742-6596/340/1/012033>`_
.. [#MAY] J Mayers, R Cywinski, 1985 *Nuclear Instruments and Methods in Physics Research* A241, A Monte Carlo Evaluation Of Analytical Multiple Scattering Corrections For Unpolarised Neutron Scattering And Polarisation Analysis Data `doi: 10.1016/0168-9002(85)90607-2 <https://doi.org/10.1016/0168-9002(85)90607-2>`_


Usage
-----


.. categories::

.. sourcelink::
