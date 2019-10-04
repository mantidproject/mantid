.. _func-RedfieldCutoff:

 ==============
 RedfieldCutoff
 ==============

 .. index:: RedfieldCutoff

 Description
 -----------

 The Redfield formula for the LF dependence on the muon spin relaxation rate.

 .. math:: \Lambda(t)= A_0\frac{2\gamma^2_\mu H^2_\text{loc}\tau}{1+\gamma^2_\mu H^2_\text{LF} \tau^2}

 where,

 :math:`A_0` is the amplitude of asymmetry,

 :math:`H_\text{loc}` is the local magnetic field,

 :math:`H_\text{LF}` is the longitudinal magnetic field applied,

 :math:`\tau` is the correlation time of muon spins,

 and its expression is given as :math:`\tau = \frac{1}{f}` where :math:`f` is the frequency of fluctuation at muon sites.

 .. plot::

    from mantid.simpleapi import FunctionWrapper
    import matplotlib.pyplot as plt
    import numpy as np
    x = np.arange(0.1,16,0.1)
    y = FunctionWrapper("RedfieldCutoff")
    fig, ax=plt.subplots()
    ax.plot(x, y(x))
    ax.set_xlabel('t($\mu$s)')
    ax.set_ylabel('A(t)')

 .. attributes::

 .. properties::

 References
 ----------

 [1]  `Takao Suzuki et al, J. Phys.: Conf. Ser. 502 012041 (2014) <https://iopscience.iop.org/article/10.1088/1742-6596/502/1/012041/pdf>`_.

 .. categories::

 .. sourcelink::
