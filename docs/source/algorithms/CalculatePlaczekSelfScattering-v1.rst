.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes an incident spectrum function and it's derivative, along with
the workspace containing spectrum info and calculates the time-of-flight Placzek
scattering correction from the workspaces material sample and detector info.
[1]_ [2]_ [3]_ For obtaining the incident spectrum from a measurement (ie beam
monitors or calibrant sample), the :ref:`FitIncidentSpectrum <algm-FitIncidentSpectrum>`
can provide the necessary inputs.

Usage
-----

**Example: calculate Placzek self scattering correction using a sample detector setup** [4]_

.. code:: python

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    # Create the workspace to hold the already corrected incident spectrum
    incident_wksp_name = 'incident_spectrum_wksp'
    binning_incident = "0.1,0.02,5.0"
    binning_for_calc = "0.2,0.02,4.0"
    binning_for_fit = "0.15,0.02,4.5"
    incident_wksp = CreateSampleWorkspace(
        OutputWorkspace=incident_wksp_name,
        NumBanks=10,
        XMin=0.1,
        XMax=5.0,
        BinWidth=0.002,
        Xunit='Wavelength')
    incident_wksp = ConvertToPointData(InputWorkspace=incident_wksp)

    # Spectrum function given in Milder et al. Eq (5)
    def incident_spectrum(wavelengths, phi_max, phi_epi, alpha, lambda_1, lambda_2,
                         lamda_t):
        delta_term = 1. / (1. + np.exp((wavelengths - lambda_1) / lambda_2))
        term_1 = phi_max * (
            lambda_t**4. / wavelengths**5.) * np.exp(-(lambda_t / wavelengths)**2.)
        term_2 = phi_epi * delta_term / (wavelengths**(1 + 2 * alpha))
        return term_1 + term_2

    # Variables for polyethlyene moderator at 300K
    phi_max = 6324
    phi_epi = 786
    alpha = 0.099
    lambda_1 = 0.67143
    lambda_2 = 0.06075
    lambda_t = 1.58

    # Add the incident spectrum to the workspace
    corrected_spectrum = incident_spectrum(
        incident_wksp.readX(0), phi_max, phi_epi, alpha, lambda_1, lambda_2, lambda_t)
    incident_wksp.setY(0, corrected_spectrum)

    incident_spectrum = FitIncidentSpectrum(
        InputWorkspace='incident_wksp',
        OutputWorkspace='fit_wksp',
        BinningForCalc=binning_for_calc,
        BinningForFit=binning_for_fit)
    SetSampleMaterial(
        InputWorkspace='incident_wksp',
        ChemicalFormula='Co')
    CalculatePlaczekSelfScattering(
        InputWorkspace='incident_wksp',
        IncidentSpecta='fit_wksp',
        OutputWorkspace='placzek_scattering',
        Version=1)


References
------------

.. [1] G. Placzek, (1952), *The Scattering of Neutrons by Systems of Heavy Nuclei*, Physical Review, Volume 86, Page 377-388 `doi: 10.1103/PhysRev.86.377 <https://doi.org/10.1103/PhysRev.86.377>`__
.. [2] J.G. Powles, (1973), *The analysis of a time-of-flight neutron diffractometer for amorphous materials: the structure of a molecule in a liquid*, Molecular Physics, Volume 26, Issue 6, Page 1325-1350, `doi: 10.1080/00268977300102521 <https://doi.org/10.1080/00268977300102521>`__
.. [3] Howe, McGreevy, and Howells, J., (1989), *The analysis of liquid structure data from time-of-flight neutron diffractometry*,Journal of Physics: Condensed Matter, Volume 1, Issue 22, pp. 3433-3451, `doi: 10.1088/0953-8984/1/22/005 <https://doi.org/10.1088/0953-8984/1/22/005>`__
.. [4] D. F. R. Mildner, B. C. Boland, R. N. Sinclair, C. G. Windsor, L. J. Bunce, and J. H. Clarke (1977) *A Cooled Polyethylene Moderator on a Pulsed Neutron Source*, Nuclear Instruments and Methods 152 437-446 `doi: 10.1016/0029-554X(78)90043-5 <https://doi.org/10.1016/0029-554X(78)90043-5>`__

.. categories::

.. sourcelink::
