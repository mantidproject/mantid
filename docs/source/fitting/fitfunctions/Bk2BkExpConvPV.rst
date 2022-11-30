.. _func-Bk2BkExpConvPV:

==============
Bk2BkExpConvPV
==============

.. index:: Bk2BkExpConvPV

Description
-----------

A back-to-back exponential convoluted pseudo-voigt function is defined as:

.. math:: F(X) = I \cdot \Omega(x)

where :math:`\Omega` is defined to be

.. math:: \Omega(x) = (1-\eta)N\left\{e^u\mathit{erfc}(y)+e^v\mathit{erfc}(z)\right\} - \frac{2N\eta}{\pi}\left\{ \Im(e^p\mathit{E}_1(p))+ \Im(e^q\mathit{E}_1(q)) \right \},

given that

.. math:: u=\frac{1}{2}\alpha\left( \alpha\sigma^{2}+2(x-X0) \right),
.. math:: y=\frac{1}{\sqrt{2\sigma^{2}}}(\alpha\sigma^{2}+x-X0),
.. math:: v=\frac{1}{2}\beta\left( \beta\sigma^{2}-2(x-X0) \right),
.. math:: z=\frac{1}{\sqrt{2\sigma^{2}}}(\beta\sigma^{2}-x+X0),
.. math:: p=\alpha(x-X0)+\frac{\alpha H}{2}i,
.. math:: q=-\beta(x-X0)+\frac{\beta H}{2}i,

.. math:: N = \frac{\alpha\beta}{2(\alpha+\beta)}.

:math:`\eta` is approximated by

.. math:: \eta = 1.36603\frac{\gamma}{H} - 0.47719\left(\frac{\gamma}{H}\right)^2 + 0.11116\left(\frac{\gamma}{H}\right)^3,

where,

.. math:: H = \gamma^5+0.07842\gamma^4H_G+4.47163\gamma^3H_G^2+2.42843\gamma^2H_G^3+2.69269\gamma H_G^4+H_G^5,
.. math:: H_G=\sqrt{8\sigma^2\log(2)}.

:math:`\mathit{erfc}` is the complementary error function and :math:`\mathit{E}_1` is the exponential integral with complex argument given by

.. math:: \mathit{erfc}(x) = 1 - \text{erf}(x) = 1 - \frac{2}{\sqrt{\pi}}\int_{0}^{x}e^{-u^{2}}du = \frac{2}{\sqrt{\pi}}\int_{x}^{\infty}e^{-u^{2}}du,
.. math:: \mathit{E}_1(z) = \int_{z}^{\infty} \frac{e^{-t}}{t}dt.

The parameters :math:`A` and :math:`B` represent the absolute value of
the exponential rise and decay constants (modelling the neutron pulse
coming from the moderator) and :math:`S` represent the standard
deviation of the gaussian. The parameter :math:`X0` is the location of
the peak; more specifically it represent the point where the
exponentially modelled neutron pulse goes from being exponentially
rising to exponentially decaying. :math:`I` is the integrated intensity.

For information about how to convert Fullprof back-to-back exponential
parameters into those used for this function see
:ref:`CreateBackToBackParameters <CreateBackToBackParameters>`.
For information about how to create parameters from a GSAS parameter file see
`CreateBackToBackParametersGSAS <http://www.mantidproject.org/CreateBackToBackParametersGSAS>`_.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
