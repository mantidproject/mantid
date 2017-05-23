.. _func-DynamicKuboToyabe:

=================
DynamicKuboToyabe
=================

.. index:: DynamicKuboToyabe

Description
-----------

Dynamic Kubo Toyabe fitting function for use by Muon scientists defined
by

.. math:: G_z \left(t\right) = g_z\left(t\right) e^{-\nu t} + \nu \int_0^t g_z\left(\tau\right) e^{-\nu\tau} G_z\left(t-\tau\right) d\tau

where :math:`g_z\left(t\right)` is the static KT function, and :math:`\nu` the muon hopping rate.

| In zero field, :math:`B_0=0`: 

.. math:: g_z\left(t\right) = \mbox{A} \Bigg[ \frac{1}{3} + \frac{2}{3} \left( 1 - {\Delta}^2 {t}^2 \right) e^{-\frac{1}{2}\Delta^2 t^2} \Bigg]

| In the presence of a longitudinal field, :math:`B_0=\omega_0 /\left(2\pi \gamma_{\mu}\right)>0`: 

.. math:: g_z\left(t\right) = \mbox{A} \Bigg[ 1 - 2\frac{\Delta^2}{\omega_0^2}\Big(1-cos(\omega_0 t)e^{-\frac{1}{2}\Delta^2 t^2}\Big) + 2\frac{\Delta^4}{\omega_0^4}\omega_0\int_0^t \sin(\omega_0\tau)e^{-\frac{1}{2}\Delta^2\tau^2}d\tau \Bigg]

DynamicKuboToyabe function has one attribute (non-fitting parameter), 'BinWidth', that sets the width of the step size between points for numerical integration. Note that 
small values will lead to long calculation times, while large values will produce less accurate results. The default value is set to 0.05, and it is allowed to vary in the range [0.001,0.1].

.. attributes::

.. properties::

.. categories::

.. sourcelink::
