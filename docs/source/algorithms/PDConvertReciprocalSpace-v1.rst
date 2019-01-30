
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The neutron diffraction is measuring the differential scattering cross section (`DCS(Q)` in the algorithm)

.. math::
    :label: CrossSection
    
    \frac{d\sigma}{d\Omega} = \frac{N}{\phi d\Omega}

Here the :math:`N` is the number of scattered neutrons in unit time in a solid angle :math:`d\Omega`, and 
:math:`\phi` is the incident neutron flux. There are other quantities of interest for experimentalists:

.. math::
    :label: SofQ
    
    S(Q) = \frac{1}{N_{s} \langle b_{coh} \rangle^2}\frac{d\sigma}{d\Omega}(Q) - \frac{\langle b_{tot}^2 \rangle - \langle b_{coh} \rangle^2}{\langle b_{coh} \rangle^2}

.. math::
    :label: FofQ
    
    F(Q) = Q [S(Q) - 1]

.. math::
    :label: FKofQ
    
    F_K(Q) = \langle b_{coh} \rangle^2 [S(Q) - 1] = \frac{\langle b_{coh} \rangle^2}{Q} F(Q)


Usage
-----

.. code-block:: python

    import wget
    import numpy as np
    import matplotlib.pyplot as plt
    from mantid.simpleapi import CreateWorkspace, SetSampleMaterial, PDConvertReciprocalSpace
    from mantid import plots

    # Grab the reciprocal data for argon
    url = "https://raw.githubusercontent.com/marshallmcdonnell/pystog/master/data/test_data/argon.reciprocal_space.dat"
    filename = wget.download(url)
    q, sq, fq_, fk_, dcs_ = np.loadtxt(filename, skiprows=2, unpack=True)

    # Convert S(Q) to Mantid wksp 
    s_of_q = CreateWorkspace(DataX=q, DataY=sq,
                           UnitX="MomentumTransfer",
                           Distribution=True)
    SetSampleMaterial(InputWorkspace=s_of_q, ChemicalFormula='Ar')
    f_of_q=PDConvertReciprocalSpace(InputWorkspace=s_of_q, From='S(Q)', To='F(Q)')
    fk_of_q=PDConvertReciprocalSpace(InputWorkspace=s_of_q, From='S(Q)', To='FK(Q)')
    dcs_of_q=PDConvertReciprocalSpace(InputWorkspace=s_of_q, From='S(Q)', To='DCS(Q)')
    
    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    ax.plot(s_of_q,'k-', label='$S(Q)$') 
    ax.plot(f_of_q,'r-', label='$F(Q)$') 
    ax.plot(fk_of_q,'b-', label='$F_K(Q)$') 
    ax.plot(dcs_of_q,'g-', label='$\frac{d\sigma}{d\Omega}(Q)$')
    ax.legend() # show the legend
    fig.show()

The output should look like:

.. figure:: /images/PDConvertReciprocalSpace.png

.. categories::

.. sourcelink::
