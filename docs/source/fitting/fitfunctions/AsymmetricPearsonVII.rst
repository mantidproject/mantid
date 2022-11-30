.. _func-AsymmetricPearsonVII:

======================
Asymmetric Pearson VII
======================

.. index:: AsymmetricPearsonVII

Description
-----------

The asymmetric Pearson VII function (sometimes it is also referred to as the split-Pearson VII function) is a function that combines
two Pearson VII distributions so to fit sharp peak curves [1]_. It is useful for analysis of X-ray diffraction peaks which consist of two separable components.

In one of the representations, the asymmetric Pearson VII function :math:`A(x)` can be written as a superposition of the "left" and "right" Pearson VII functions :math:`P_L(x)` and :math:`P_R(x)` [2]_,

.. math:: A(x) = P_L(x) \, \theta(x_0 - x) + P_R(x) \, \theta(x - x_0) = P_L(x) \, \theta(x_0 - x) + P_R(x) \, \left(1 - \theta(x_0 - x) \right),

where :math:`x_0` is the value when the function changes between the left and right Pearson VII, and this is usually chosen to be the peak centre. Moreover, :math:`\theta` denotes the Heavyside step function with :math:`\theta(y \ge 0) = 1` and :math:`\theta(y < 0)=0`, and :math:`P(x)` denotes the Pearson VII function. This function takes four additional parameters and in general is defined as

.. math:: P(x, h, x_0, \Gamma, m) = \frac{h}{\left(1 + \left[ \frac{2 (x-x_0) \sqrt{2^{1/m}-1}}{\Gamma} \right]^2 \right)^m} \, .

The four free parameters that the Pearson VII function :math:`P(x, h, x_0, \Gamma, m)` takes represent the following quantities

-  :math:`h` - the height of the peak
-  :math:`x_0` - the location (centre) of the peak
-  :math:`\Gamma` - the full width at half maximum
-  :math:`m` - the function's shape parameter

The parameter :math:`m` can take values in the range :math:`[0, \infty)` and it determines whether :math:`P(x, h, x_0, \Gamma, m)` is more close to a Lorentzian or a Gaussian. When :math:`m=1` it's a Lorentzian, whereas when :math:`m \rightarrow \infty` it's a Gaussian. The "left" and "right" Pearson VII functions :math:`P_L(x)` and :math:`P_R(x)` take identical parameters :math:`h, x_0, \Gamma`, but diffent :math:`m`, so that

.. math:: P_L \equiv P(x, h, x_0, \Gamma, m_L),
.. math:: P_R \equiv P(x, h, x_0, \Gamma, m_R).

In other words, it is assumed that the asymmetric Pearson VII function uses two halves of the Pearson VII with a common  peak hight, peak centre, and full width at half maximum. As a result, the asymmetric Pearson VII distribution takes five parameters: :math:`h, x_0, \Gamma, m_L, m_R`. For the sake of stability of the numerical implementation of asymmetric Pearson VII distribution in Mantid, one operates with the weight parameter :math:`w \equiv 1/\Gamma`, i. e. :math:`A = A(x, h, x_0, w, m_L, m_R)`.

Analytic Derivatives
--------------------

Below is the list of analytic derivatives of :math:`P = P(x, h, x_0, w, m)` with respect to :math:`h, x_0, w, m`:

.. math:: \frac{\partial P}{\partial h} = \frac{1}{d^m},

.. math:: \frac{\partial P}{\partial x_0} = \frac{8 h m (x - x_0) \left( 2^{1/m} - 1 \right) w^2}{d^{m + 1}},

.. math:: \frac{\partial P}{\partial w} = - \frac{8 h m (x - x_0)^2 \left( 2^{1/m} - 1 \right) w}{d^{m + 1}},

.. math:: \frac{\partial P}{\partial m} = \frac{h}{d^{m + 1}} \left[ \frac{4 (x - x_0)^2 w^2 2^{1/m} \ln 2}{m d} - \ln \frac{1}{d} \right],

where

.. math:: d \equiv \frac{1}{\left( 1 + 4 \left( x - x_0 \right)^2 \left( 2^{1/m} - 1 \right) w^2 \right)} \, .

Asymptotic Behavior
-------------------

The numerical calculation of  :math:`P(x, h, x_0, w, m)` and its partial derivatives is complicated by singularities as :math:`m \rightarrow 0`. For this reason, one needs to operate with a limit of the corresponding expressions:

.. math:: \lim_{m \rightarrow 0} P(x, h, x_0, w, m) = \frac{h}{2} \, ,

.. math:: \lim_{m \rightarrow 0} \, \frac{\partial P(x, h, x_0, w, m)}{\partial h} = \frac{1}{2} \, ,

.. math:: \lim_{m \rightarrow 0} \, \frac{\partial P(x, h, x_0, w, m)}{\partial x_0} = 0 \, ,

.. math:: \lim_{m \rightarrow 0} \, \frac{\partial P(x, h, x_0, w, m)}{\partial w} = 0 \, ,

.. math:: \lim_{m \rightarrow 0} \, \frac{\partial P(x, h, x_0, w, m)}{\partial m} = - \frac{h}{2} \, \ln \left[ 4(x-x_0)^2 w^2 \right] \, .


The figure below illustrates the comparison of shapes between various symmetric [3]_ and asymmetric Pearson VII distributions plotted for :math:`h_0 = 0.7297`, :math:`x_{00} = 0.0035`, :math:`w_0 = 5.7215`.

.. figure:: /images/AsymmetricPearsonVII.png
   :alt: AsymmetricPearsonVII.png

.. attributes::

.. properties::

References
----------

.. [1] Gupta, S. K. Peak Decomposition using Pearson Type VII Function, `J. Appl. Cryst. (1998). 31, 474-476 <https://doi.org/10.1107/S0021889897011047>`__

.. [2] Lifshin, E. (Ed.). (1999), X-ray Characterization of Materials, WILEY-VCH, `<https://doi.org/10.1002/9783527613748>`__

.. [3] `<http://pd.chem.ucl.ac.uk/pdnn/peaks/pvii.htm>`__

.. categories::

.. sourcelink::
