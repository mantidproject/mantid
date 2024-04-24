
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
:math:`\phi` is the incident neutron flux. The algorithm supports the following conversions:

.. math::
    :label: SofQ

    S(Q) = \frac{1}{N_{s} \langle b_{coh} \rangle^2}\frac{d\sigma}{d\Omega}(Q) - \frac{\langle b_{tot}^2 \rangle - \langle b_{coh} \rangle^2}{\langle b_{coh} \rangle^2}

.. math::
    :label: FofQ

    F(Q) = Q [S(Q) - 1]

.. math::
    :label: FKofQ

    F_K(Q) = \langle b_{coh} \rangle^2 [S(Q) - 1] = \frac{\langle b_{coh} \rangle^2}{Q} F(Q)

where :math:`N_s` is the number of scatters in the sample and both :math:`\langle b_{tot}^2 \rangle` and :math:`\langle b_{coh} \rangle^2` are defined in the :ref:`Materials concept page <Materials>`.

NOTE: This algorithm requires that :ref:`algm-SetSampleMaterial` is called prior in order to determine the :math:`\langle b_{tot}^2 \rangle` and :math:`\langle b_{coh} \rangle^2` terms.

PyStoG
------
This algorithm uses the external project `PyStoG <https://pystog.readthedocs.io/en/latest/>`_ and specifically uses the :class:`pystog.converter.Converter` object. To modify the underlying algorithms, the following functions are used for the conversions.

- :math:`\frac{d\sigma}{d\Omega}(Q)` conversions are:
    - To :math:`F(Q)` see :meth:`pystog.converter.Converter.DCS_to_F`
    - To :math:`F_K(Q)` see :meth:`pystog.converter.Converter.DCS_to_FK`
    - To :math:`S(Q)` see :meth:`pystog.converter.Converter.DCS_to_S`

- :math:`S(Q)` conversions are:
    - To :math:`F(Q)` see :meth:`pystog.converter.Converter.S_to_F`
    - To :math:`F_K(Q)` see :meth:`pystog.converter.Converter.S_to_FK`
    - To :math:`\frac{d\sigma}{d\Omega}(Q)` see :meth:`pystog.converter.Converter.S_to_DCS`

- :math:`F(Q)` conversions are:
    - To :math:`\frac{d\sigma}{d\Omega}(Q)` see :meth:`pystog.converter.Converter.F_to_DCS`
    - To :math:`F_K(Q)` see :meth:`pystog.converter.Converter.F_to_FK`
    - To :math:`S(Q)` see :meth:`pystog.converter.Converter.F_to_S`

- :math:`F_K(Q)` conversions are:
    - To :math:`\frac{d\sigma}{d\Omega}(Q)` see :meth:`pystog.converter.Converter.FK_to_DCS`
    - To :math:`F(Q)` see :meth:`pystog.converter.Converter.FK_to_F`
    - To :math:`S(Q)` see :meth:`pystog.converter.Converter.FK_to_S`

Usage
-----

.. code-block:: python

    import wget
    import numpy as np
    import matplotlib.pyplot as plt
    from mantid.simpleapi import CreateWorkspace, SetSampleMaterial, PDConvertReciprocalSpace
    from mantid import plots

    # Grab the reciprocal data for argon
    url = "https://raw.githubusercontent.com/marshallmcdonnell/pystog/master/tests/test_data/argon.reciprocal_space.dat"
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
    ax.plot(dcs_of_q,'g-', label=r'$d\sigma / d\Omega(Q)$')
    ax.legend() # show the legend
    fig.show()

The output should look like:

.. figure:: /images/PDConvertReciprocalSpace.png

.. categories::

.. sourcelink::
