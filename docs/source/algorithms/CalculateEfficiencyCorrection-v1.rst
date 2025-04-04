.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm calculates the detection efficiency correction, :math:`\epsilon`, defined by:

.. math::
    :label: efficiency

    \epsilon &= 1-e^{-\rho T \sigma (\lambda)} \\
             &= 1-e^{-\rho_{A} \sigma (\lambda)} \\
             &= 1-e^{-\alpha \lambda}

and output correction is given as:

.. math::
    :label: efficiency_correction

    \frac{1}{\epsilon} = \frac{1}{1-e^{-\alpha \lambda}}

where

- :math:`\rho` is the number density in atoms/:math:`\AA^3`
- :math:`T` is a sample thickness given in cm
- :math:`\lambda_{ref}` = 1.7982 :math:`\AA`,
- :math:`\sigma (\lambda)` is the wavelength-dependent cross-section which is either:

  - :math:`\sigma (\lambda) = \sigma_a (\lambda_{ref}) \left( \frac{\lambda}{\lambda_{ref}} \right)` for ``XSectionType`` == ``AttenuationXSection`` where :math:`\sigma_a` is the absorption cross-section in units of barns

  - or :math:`\sigma (\lambda) = \sigma_s + \sigma_a (\lambda_{ref}) \left( \frac{\lambda}{\lambda_{ref}} \right)` for ``XSectionType`` == ``TotalXSection`` where :math:`\sigma_s` is the total scattering cross-section in units of barns

- :math:`\rho_{A}` is the area density (:math:`\rho_{A}=\rho * T`) in units of atoms*cm/:math:`\AA^3`,
- :math:`\alpha = \rho_{A} \cdot \frac{\sigma (\lambda_{ref})}{\lambda_{ref}} = \rho \cdot T \cdot \frac{\sigma (\lambda_{ref})}{\lambda_{ref}}` in units of 1/:math:`\AA`.
- :math:`\lambda` is in units of :math:`\AA`.

NOTE: :math:`1 \AA^2 = 10^{8}` barns and :math:`1 \AA = 10^{-8}` cm.

The optional inputs into the algorithm are to input either:
  1. The ``Alpha`` parameter
  2. The ``Density`` and ``ChemicalFormula`` to calculate the :math:`\sigma(\lambda_{ref})` term.
  3. The ``MeasuredEfficiency``, an optional ``MeasuredEfficiencyWavelength``, and ``ChemicalFormula`` to calculate the :math:`\sigma(\lambda_{ref})` term.

The ``MeasuredEfficiency`` is the :math:`\epsilon` term measured at a specific wavelength, :math:`\lambda_{\epsilon}`, which is specified by ``MeasuredEfficiencyWavelength``. This helps
if the efficiency has been directly measured experimentally at a given wavelength. This will calculate the
:math:`\rho * T` term, where it will be either:

- :math:`\rho * T = - \ln(1-\epsilon) \frac{1}{ \frac{\lambda_{\epsilon} \sigma (\lambda_{ref})}{\lambda_{ref}}}` for ``XSectionType`` == ``AttenuationXSection``
- :math:`\rho * T = - \ln(1-\epsilon) \frac{1}{ \sigma_s + \frac{\lambda_{\epsilon} \sigma (\lambda_{ref})}{\lambda_{ref}}}` for ``XSectionType`` == ``TotalXSection``

For the ``XSectionType``, if the efficiency correction is applied to a beam monitor to determine the incident spectrum, then the ``AttenuationXSection`` option should be used. This is due to the fact that scatter events do not lead to neutrons that will be in the incident beam. If the efficiency correction is to be used similar to a transmission measurement for an actual sample measurement, such as in :ref:`algm-CalculateSampleTransmission-v1`, then the ``TotalXSection`` should be used to include both types of events.

Usage
-----
**Example - Basics of running CalculateEfficiencyCorrection with Alpha.**

.. testcode:: ExBasicCalcualteEfficiencyCorrectionWithAlpha

    # Create an exponentially decaying function in wavelength to simulate measured sample
    input_wksp = CreateSampleWorkspace(WorkspaceType="Event", Function="User Defined",
                                       UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
                                       Xmin=0.2, Xmax=4.0, BinWidth=0.01, XUnit="Wavelength",
                                       NumEvents=10000, NumBanks=1, BankPixelWidth=1)

    # Calculate the efficiency correction
    corr_wksp = CalculateEfficiencyCorrection(InputWorkspace=input_wksp, Alpha=0.5)
    corr_wksp_with_wave_range = CalculateEfficiencyCorrection(WavelengthRange="0.2,0.01,4.0", Alpha=0.5)

    # Apply the efficiency correction to the measured spectrum
    input_wksp = ConvertToPointData(InputWorkspace=input_wksp)
    output_wksp = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp)
    output_wksp_with_wave_range = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp_with_wave_range)

    print('Input workspace: {}'.format(input_wksp.readY(0)[:5]))
    print('Correction workspace: {}'.format(corr_wksp.readY(0)[:5]))
    print('Output workspace: {}'.format(output_wksp.readY(0)[:5]))
    print('Output workspace using WavelengthRange: {}'.format(output_wksp_with_wave_range.readY(0)[:5]))

Output:

.. testoutput:: ExBasicCalcualteEfficiencyCorrectionWithAlpha

    Input workspace: [40. 40. 40. 40. 40.]
    Correction workspace: [10.26463773  9.81128219  9.39826191  9.02042771  8.67347109]
    Output workspace: [410.58550929 392.45128759 375.93047648 360.81710849 346.93884349]
    Output workspace using WavelengthRange: [410.58550929 392.45128759 375.93047648 360.81710849 346.93884349]

**Example - Basics of running CalculateEfficiencyCorrection with Density and ChemicalFormula.**

.. testcode:: ExBasicCalcualteEfficiencyCorrectionWithDensity

    # Create an exponentially decaying function in wavelength to simulate measured sample
    input_wksp = CreateSampleWorkspace(WorkspaceType="Event", Function="User Defined",
                                       UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
                                       Xmin=0.2, Xmax=4.0, BinWidth=0.01, XUnit="Wavelength",
                                       NumEvents=10000, NumBanks=1, BankPixelWidth=1)

    # Calculate the efficiency correction
    corr_wksp = CalculateEfficiencyCorrection(InputWorkspace=input_wksp,
                                              Density=6.11,
                                              ChemicalFormula="V")
    corr_wksp_with_wave_range = CalculateEfficiencyCorrection(WavelengthRange="0.2,0.01,4.0",
                                                              Density=6.11,
                                                              ChemicalFormula="V")

    # Apply the efficiency correction to the measured spectrum
    input_wksp = ConvertToPointData(InputWorkspace=input_wksp)
    output_wksp = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp)
    output_wksp_with_wave_range = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp_with_wave_range)

    print('Input workspace: {}'.format(input_wksp.readY(0)[:5]))
    print('Correction workspace: {}'.format(corr_wksp.readY(0)[:5]))
    print('Output workspace: {}'.format(output_wksp.readY(0)[:5]))
    print('Output workspace using WavelengthRange: {}'.format(output_wksp_with_wave_range.readY(0)[:5]))

Output:

.. testoutput:: ExBasicCalcualteEfficiencyCorrectionWithDensity

    Input workspace: [40. 40. 40. 40. 40.]
    Correction workspace: [24.40910309 23.29738394 22.28449939 21.35783225 20.50682528]
    Output workspace: [976.3641235  931.8953577  891.37997557 854.31328983 820.2730111 ]
    Output workspace using WavelengthRange: [976.3641235  931.8953577  891.37997557 854.31328983 820.2730111 ]

**Example - Basics of running CalculateEfficiencyCorrection with MeasuredEfficiency and ChemicalFormula.**

.. testcode:: ExBasicCalcualteEfficiencyCorrectionWithEfficiency

    # Create an exponentially decaying function in wavelength to simulate measured sample
    input_wksp = CreateSampleWorkspace(WorkspaceType="Event", Function="User Defined",
                                       UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
                                       Xmin=0.2, Xmax=4.0, BinWidth=0.01, XUnit="Wavelength",
                                       NumEvents=10000, NumBanks=1, BankPixelWidth=1)

    # Calculate the efficiency correction
    corr_wksp = CalculateEfficiencyCorrection(InputWorkspace=input_wksp,
                                              MeasuredEfficiency=1e-2,
                                              ChemicalFormula="(He3)")

    corr_wksp_with_wave_range = CalculateEfficiencyCorrection(WavelengthRange="0.2,0.01,4.0",
                                                              MeasuredEfficiency=1e-2,
                                                              ChemicalFormula="(He3)")


    # Apply the efficiency correction to the measured spectrum
    input_wksp = ConvertToPointData(InputWorkspace=input_wksp)
    output_wksp = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp)
    output_wksp_with_wave_range = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp_with_wave_range)

    print('Input workspace: {}'.format(input_wksp.readY(0)[:5]))
    print('Correction workspace: {}'.format(corr_wksp.readY(0)[:5]))
    print('Output workspace: {}'.format(output_wksp.readY(0)[:5]))
    print('Output workspace using WavelengthRange: {}'.format(output_wksp_with_wave_range.readY(0)[:5]))

Output:

.. testoutput:: ExBasicCalcualteEfficiencyCorrectionWithEfficiency

    Input workspace: [40. 40. 40. 40. 40.]
    Correction workspace: [873.27762699 832.68332786 795.69741128 761.85923269 730.78335476]
    Output workspace: [34931.10507965 33307.33311431 31827.89645133 30474.36930745
     29231.33419051]
    Output workspace using WavelengthRange: [34931.10507965 33307.33311431 31827.89645133 30474.36930745
     29231.33419051]

**Example - Basics of running CalculateEfficiencyCorrection with MeasuredEfficiency and ChemicalFormula using the total cross section.**

.. testcode:: ExBasicCalcualteEfficiencyCorrectionWithEfficiency

    # Create an exponentially decaying function in wavelength to simulate measured sample
    input_wksp = CreateSampleWorkspace(WorkspaceType="Event", Function="User Defined",
                                       UserDefinedFunction="name=ExpDecay, Height=100, Lifetime=4",
                                       Xmin=0.2, Xmax=4.0, BinWidth=0.01, XUnit="Wavelength",
                                       NumEvents=10000, NumBanks=1, BankPixelWidth=1)

    # Calculate the efficiency correction
    corr_wksp = CalculateEfficiencyCorrection(InputWorkspace=input_wksp,
                                              MeasuredEfficiency=1e-2,
                                              ChemicalFormula="(He3)",
                                              XSectionType="TotalXSection")

    corr_wksp_with_wave_range = CalculateEfficiencyCorrection(WavelengthRange="0.2,0.01,4.0",
                                                              MeasuredEfficiency=1e-2,
                                                              ChemicalFormula="(He3)",
                                                              XSectionType="TotalXSection")


    # Apply the efficiency correction to the measured spectrum
    input_wksp = ConvertToPointData(InputWorkspace=input_wksp)
    output_wksp = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp)
    output_wksp_with_wave_range = Multiply(LHSWorkspace=input_wksp, RHSWorkspace=corr_wksp_with_wave_range)

    print('Input workspace: {}'.format(input_wksp.readY(0)[:5]))
    print('Correction workspace: {}'.format(corr_wksp.readY(0)[:5]))
    print('Output workspace: {}'.format(output_wksp.readY(0)[:5]))
    print('Output workspace using WavelengthRange: {}'.format(output_wksp_with_wave_range.readY(0)[:5]))

Output:

.. testoutput:: ExBasicCalcualteEfficiencyCorrectionWithEfficiency

    Input workspace: [40. 40. 40. 40. 40.]
    Correction workspace: [865.7208838  825.85320701 789.49774383 756.20995361 725.61727932]
    Output workspace: [34628.83535201 33034.12828025 31579.90975329 30248.39814428
     29024.69117275]
    Output workspace using WavelengthRange: [34628.83535201 33034.12828025 31579.90975329 30248.39814428
     29024.69117275]

The transmission of a sample can be measured as :math:`e^{-\rho T \sigma_t (\lambda)}` where :math:`\sigma_t (\lambda) = \sigma_s + \sigma_a (\lambda)` is the total cross-section. This can be calculated directly by the :ref:`algm-CalculateSampleTransmission-v1` algorithm. Yet, we can also back out the transmission with the ``CalculateEfficiencyCorrection`` algorithm. The example below shows how:

**Example - Transmission using the CalculateEfficiencyCorrection and CalculateSampleTransmission comparison.**

.. testcode:: ExTransmissionCalcualteEfficiencyCorrection

    ws = CalculateSampleTransmission(WavelengthRange='2.0, 0.1, 10.0',
                                     ChemicalFormula='H2-O')
    print('Transmission: {} ...'.format(ws.readY(0)[:3]))

    corr_wksp = CalculateEfficiencyCorrection(WavelengthRange="2.0, 0.1, 10.0",
                                              Density=0.1,
                                              Thickness=0.1,
                                              ChemicalFormula="H2-O",
                                              XSectionType="TotalXSection")
    dataX = corr_wksp.readX(0)
    dataY = np.ones(len(corr_wksp.readX(0)))
    ones = CreateWorkspace(dataX, dataY, UnitX="Wavelength")
    efficiency = Divide(LHSWorkspace=ones, RHSWorkspace=corr_wksp) # 1 + -1 * transmission
    negative_trans = Minus(LHSWorkspace=efficiency, RHSWorkspace=ones) # -1 * transmission
    transmission = Multiply(LHSWOrkspace=negative_trans, RHSWorkspace=-1.*ones)
    print('Transmission using efficiency correction: {} ...'.format(transmission.readY(0)[:3]))

Output:

.. testoutput:: ExTransmissionCalcualteEfficiencyCorrection

    Transmission: [0.94506317 0.94505148 0.94503979] ...
    Transmission using efficiency correction: [0.9450632  0.94505151 0.94503982] ...

The discrepancies are due to the differenc in :math:`\lambda_{ref}` = 1.7982 :math:`\AA` in ``CalculateEfficiencyCorrection``, consistent with `ReferenceLambda <https://github.com/mantidproject/mantid/blob/32ed0b2cbbe4fbfb230570d5a53032f6101743de/Framework/Kernel/src/NeutronAtom.cpp#L23>`_ where :ref:`algm-CalculateSampleTransmission-v1`  uses :math:`\lambda_{ref}` = 1.798 :math:`\AA`.

**Example - Running CalculateEfficiencyCorrection for incident spectrum.**

To model the incident spectrum of polyethylene moderators, the following function is used to
join the exponential decay of the epithermal flux  to the Maxwellian distribution of the thermal flux [1]_:

.. math::
    :label: incident_spectrum

    \phi(\lambda) = \phi_{max} \frac{\lambda_T^4}{\lambda^5} \mathrm{e}^{-(\lambda_T / \lambda)^2} + \phi_{epi} \frac{\Delta(\lambda_T / \lambda)}{\lambda^{1+2\alpha}}

To determine this incident spectrum experimentally, one must make a measurement either via using a sample measurement such as vanadium [1]_ or using beam monitors. [2]_ [3]_ In either case, an efficiency correction must be applied to the measured spectrum to obtain the actual incident spectrum. This incident spectrum is a crucial part of calculating Placzek recoil sample corrections. [4]_

From Eq. :eq:`incident_spectrum`, the parameters vary based on the moderator material. For a polyethlyene moderator at a temperature of 300K, the following parameters have been used to accurately model the incident spectrum. [1]_ The parameter labels, variables used in the following code example, and values for the parameters are given in the table below:

+--------------------+-------------+-----------------------------+
| Parameter          | Variables   | Polyethlyene 300K (ambient) |
+====================+=============+=============================+
| :math:`\phi_{max}` | ``phiMax``  | 6324                        |
+--------------------+-------------+-----------------------------+
| :math:`\phi_{epi}` | ``phiEpi``  | 786                         |
+--------------------+-------------+-----------------------------+
| :math:`\alpha`     | ``alpha``   | 0.099                       |
+--------------------+-------------+-----------------------------+
| :math:`\lambda_1`  | ``lambda1`` | 0.67143                     |
+--------------------+-------------+-----------------------------+
| :math:`\lambda_2`  | ``lambda2`` | 0.06075                     |
+--------------------+-------------+-----------------------------+
| :math:`\lambda_T`  | ``lambdaT`` | 1.58 :math:`\AA`            |
+--------------------+-------------+-----------------------------+

To first back out the measured spectrum of Milder et al. [1]_, the incident spectrum for polyethylene at 300K using Eq. :eq:`incident_spectrum` is obtained, then the efficiency correction is calculated, and then the incident spectrum is divided by this correction to back out what was originally measured. Then, the correction is applied by multiplying it by the measured spectrum to get back to the corrected incident spectrum to demonstrate how this is regularly apply this to a measured spectrum:

.. testcode:: ExIncidentSpectrum

    # Create the workspace to hold the already corrected incident spectrum
    incident_wksp_name = 'incident_spectrum_wksp'
    binning = "%s,%s,%s" % (0.2,0.01,4.0)
    incident_wksp = CreateWorkspace(OutputWorkspace=incident_wksp_name,
                                    NSpec=1, DataX=[0], DataY=[0],
                                    UnitX='Wavelength',
                                    VerticalAxisUnit='Text',
                                    VerticalAxisValues='IncidentSpectrum')
    incident_wksp = Rebin(InputWorkspace=incident_wksp, Params=binning)
    incident_wksp = ConvertToPointData(InputWorkspace=incident_wksp)

    # Spectrum function given in Milder et al. Eq (5)
    def incidentSpectrum(wavelengths, phiMax, phiEpi, alpha, lambda1, lambda2, lambdaT):
        deltaTerm =  1. / (1. + np.exp((wavelengths - lambda1) / lambda2))
        term1 = phiMax * (lambdaT**4. / wavelengths**5.) * np.exp(-(lambdaT / wavelengths)**2.)
        term2 = phiEpi * deltaTerm / (wavelengths**(1 + 2 * alpha))
        return term1 + term2

    # Variables for polyethlyene moderator at 300K
    phiMax  = 6324
    phiEpi  = 786
    alpha   = 0.099
    lambda1 = 0.67143
    lambda2 = 0.06075
    lambdaT = 1.58

    # Add the incident spectrum to the workspace
    corrected_spectrum = incidentSpectrum(incident_wksp.readX(0),
                                          phiMax, phiEpi, alpha,
                                          lambda1, lambda2, lambdaT)
    incident_wksp.setY(0, corrected_spectrum)

    # Calculate the efficiency correction for Alpha=0.693 and back calculate measured spectrum
    eff_wksp = CalculateEfficiencyCorrection(InputWorkspace=incident_wksp, Alpha=0.693)
    measured_wksp = Divide(LHSWorkspace=incident_wksp, RHSWorkspace=eff_wksp)

    # Re-applying the correction to the measured data (how to normally use it)
    eff2_wksp = CalculateEfficiencyCorrection(InputWorkspace=measured_wksp, Alpha=0.693)
    recorrected_wksp = Multiply(LHSWorkspace=measured_wksp, RHSWorkspace=eff2_wksp)

    print('Measured incident spectrum: {}'.format(measured_wksp.readY(0)[:5]))
    print('Corrected incident spectrum: {}'.format(incident_wksp.readY(0)[:5]))
    print('Re-corrected incident spectrum: {}'.format(recorrected_wksp.readY(0)[:5]))

Output:

.. testoutput:: ExIncidentSpectrum

   Measured incident spectrum: [694.61415533 685.71520053 677.21326605 669.0696332  661.25022644]
   Corrected incident spectrum: [5244.9385468  4953.63834159 4690.60136547 4451.98728342 4234.6092648 ]
   Re-corrected incident spectrum: [5244.9385468  4953.63834159 4690.60136547 4451.98728342 4234.6092648 ]

References
------------

.. [1] D. F. R. Mildner, B. C. Boland, R. N. Sinclair, C. G. Windsor, L. J. Bunce, and J. H. Clarke (1977) *A Cooled Polyethylene Moderator on a Pulsed Neutron Source*, Nuclear Instruments and Methods 152 437-446 `doi: 10.1016/0029-554X(78)90043-5 <https://doi.org/10.1016/0029-554X(78)90043-5>`__
.. [2] J. P. Hodges, J. D. Jorgensen, S. Short, D. N. Argyiou, and J. W. Richardson, Jr.  *Incident Spectrum Determination for Time-of-Flight Neutron Powder Diffraction Data Analysis* ICANS 14th Meeting of the International Collaboration on Advanced Neutron Sources 813-822 `link to paper <http://www.neutronresearch.com/parch/1998/01/199801008130.pdf>`__
.. [3] F. Issa, A. Khaplanov, R. Hall-Wilton, I. Llamas, M. Dalseth Ricktor, S. R. Brattheim, and H. Perrey (2017) *Characterization of Thermal Neutron Beam Monitors* Physical Review Accelerators and Beams 20 092801 `doi: 10.1103/PhysRevAccelBeams.20.092801 <https://doi.org/10.1103/PhysRevAccelBeams.20.092801>`__
.. [4] W. S. Howells (1983) *On the Choice of Moderator for Liquids Diffractometer on a Pulsed Neutron Source*, Nuclear Instruments and Methods in Physics Research 223 141-146 `doi: 10.1016/0167-5087(84)90256-4 <https://doi.org/10.1016/0167-5087(84)90256-4>`__


.. categories::

.. sourcelink::
