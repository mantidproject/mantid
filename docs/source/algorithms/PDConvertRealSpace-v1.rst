
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The neutron diffraction is measuring the differential scattering cross section. This can be
converted to the structure factor :math:`S(Q)`. Using the :ref:`PDFFourierTransform
<algm-PDFFourierTransform>` algorithm, one can obtain the pair distribution function, :math:`G(r)`:

.. math::
    :label: CapGofr

    G(r)=\frac{2}{\pi}\int_0^\infty Q[S(Q)-1]\sin(Qr) dQ


One can transform between this quantity and :math:`GK(r)` or :math:`g(r)` using:

.. math::
    :label: GKofr

    GK(r)=\frac{\langle b_{coh} \rangle^2}{4\pi r\rho_0}G(r)

.. math::
    :label: gofr

    g(r)=\frac{G(r)}{4\pi r\rho_0}+1

where :math:`\rho_0` is the sample number density and :math:`\langle b_{coh} \rangle^2` is defined in the :ref:`Materials concept page <Materials>`.

NOTE: This algorithm requires that :ref:`algm-SetSampleMaterial` is called prior in order to determine the :math:`\rho_0` and :math:`\langle b_{coh} \rangle^2` terms.

PyStoG
------
This algorithm uses the external project `PyStoG <https://pystog.readthedocs.io/en/latest/>`_ and specifically uses the :class:`pystog.converter.Converter` object. To modify the underlying algorithms, the following functions are used for the conversions.

- :math:`G(r)` conversions are:
    - To :math:`G_K(r)` see :meth:`pystog.converter.Converter.G_to_GK`
    - To :math:`g(r)` see :meth:`pystog.converter.Converter.G_to_g`

- :math:`G_K(r)` conversions are:
    - To :math:`G(r)` see :meth:`pystog.converter.Converter.GK_to_G`
    - To :math:`g(r)` see :meth:`pystog.converter.Converter.GK_to_g`

- :math:`g(r)` conversions are:
    - To :math:`G(r)` see :meth:`pystog.converter.Converter.g_to_G`
    - To :math:`GK(r)` see :meth:`pystog.converter.Converter.g_to_GK`


Usage
-----

.. code-block:: python

    import wget
    import numpy as np
    import matplotlib.pyplot as plt
    from mantid.simpleapi import CreateWorkspace
    # Grab the real data for argon
    url = "https://raw.githubusercontent.com/marshallmcdonnell/pystog/master/tests/test_data/argon.real_space.dat"
    filename = wget.download(url)
    r, gofr, GofR_, GKofR_ = np.loadtxt(filename, skiprows=2, unpack=True)

    # Convert gofr to Mantid wksp
    g_of_r = CreateWorkspace(DataX=r, DataY=gofr,
                             UnitX="Angstrom",
                             Distribution=True)
    SetSampleMaterial(InputWorkspace=g_of_r, ChemicalFormula='Ar')
    bigG_of_r=PDConvertRealSpace(InputWorkspace=g_of_r, From='g(r)', To='G(r)')
    GK_of_r=PDConvertRealSpace(InputWorkspace=g_of_r, From='g(r)', To='GK(r)')

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    ax.plot(g_of_r,'k-', label='$g(r)$')
    ax.plot(bigG_of_r,'r-', label='$G(r)$')
    ax.plot(GK_of_r,'b-', label='$G_K(r)$')
    ax.legend() # show the legend
    ax.set_xlabel(r'$r(\AA)$')
    fig.show()

The output should look like:

.. figure:: /images/PDConvertRealSpace.png

.. categories::

.. sourcelink::
