.. _func-Redfield:

=================
Redfield
=================

.. index:: Redfield

Description
-----------

The Redfield formula for the Longitudinal Field (LF) dependence of the muon spin relaxation rate, :math:`\Lambda`, given in units of
:math:`\mu s^{-1}`, with the applied longitudinal magnetic field, for given local magnetic field (:math:`H_\text{loc}`) and correlation time of
fluctuations at muon spin sites (:math:`\tau`) is:

.. math:: \Lambda(t)= \frac{2\gamma^2_\mu H^2_\text{loc}\tau}{1+\gamma^2_\mu H^2_\text{LF} \tau^2}

where,

:math:`H_\text{loc}` is the local magnetic field, in Gauss,

:math:`H_\text{LF}` is the applied longitudinal magnetic field, in Gauss,

:math:`\tau` is the muon spin correlation time, in microseconds, with expression given as :math:`\tau = \frac{1}{f}`
where :math:`f` is the frequency of fluctuation at muon sites.

And :math:`\gamma_\mu` is the gyromagnetic ratio of the muon spin, given in units of :math:`[rad]x\frac{MHz}{Gauss}`

.. plot::

	from mantid.simpleapi import FunctionWrapper
	import matplotlib.pyplot as plt
	import numpy as np
	x = np.logspace(-1, 3.4, num = 200)
	h = np.linspace(0.05, 0.2, 5)
	y = []
	Redfield = FunctionWrapper("Redfield")
	for hloc in h:
		y.append(Redfield(x, hloc))

	fig, ax = plt.subplots()
	ax.plot(x, np.array(y).T, label=['{:.2f}'.format(item) for item in h])
	ax.legend(title='$H_{loc}$ (G)')
	ax.set_xscale("log")
	ax.set_xlabel('$H_{LF}$ (Gauss)')
	ax.set_ylabel('$\Lambda(\mu s^{-1})$')

.. attributes::

.. properties::

References
----------

[1]  `Takao Suzuki et al, J. Phys.: Conf. Ser. 502 012041 (2014) <https://iopscience.iop.org/article/10.1088/1742-6596/502/1/012041/pdf>`_.

.. categories::

.. sourcelink::
