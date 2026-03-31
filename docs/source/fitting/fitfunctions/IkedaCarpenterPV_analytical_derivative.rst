.. _func-IkedaCarpenterPV-derivative:

=========================================
IkedaCarpenterPV analytical derivative
=========================================

Overview
--------

This page records the analytical derivative of the implemented
:ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` peak shape.
This derivation has been transcribed from handwritten notes produced during development, so there
may be notation error, Please flag if you find an issue.
The notation has been used for convenience with comparison to the code in
``IkedaCarpenterPV.cpp`` as well as clarity for distinguishing between variables, so there are some differences
in the symbols used compared with :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>`.

The Mantid implementation of :ref:`IkedaCarpenterPV <func-IkedaCarpenterPV>` evaluates

.. math::

   f = I Q \left[(1-\eta)G - \frac{2\eta}{\pi}L\right].

Natural parameters and helper definitions
-----------------------------------------

It is convenient to derive the Jacobian first with respect to the natural
parameters

.. math::

   (I, \alpha, \beta, R, S, H, \eta, \Delta),

and then chain these back to the Mantid parameters.

The wavelength-dependent moderator parameters are

.. math::

   \alpha = \frac{1}{A_0 + \lambda A_1},

.. math::

   \beta = \frac{1}{B_0},

.. math::

   R = \exp\left(-\frac{81.799}{\kappa \lambda^2}\right),

.. math::

   \Delta = x_i - X_0,

where :math:`A_0 = \mathrm{Alpha0}`, :math:`A_1 = \mathrm{Alpha1}`,
:math:`B_0 = \mathrm{Beta0}`, :math:`\kappa = \mathrm{Kappa}`,
:math:`x_i` is the current x-value, and :math:`\lambda` is the wavelength at
that x-value.

As in the implementation, define

.. math::

   k = 0.05,

.. math::

   a_u = \alpha(1-k), \qquad a_v = \alpha(1+k), \qquad a_s = \alpha, \qquad a_r = \beta,

.. math::

   x = a_u - \beta, \qquad y = \alpha - \beta, \qquad z = a_v - \beta.

The overall prefactor is

.. math::

   Q = \frac{1}{4} \alpha \frac{1-k^2}{k^2}.

The branch coefficients are

.. math::

   N_u = 1 - R\frac{a_u}{x},

.. math::

   N_v = 1 - R\frac{a_v}{z},

.. math::

   N_s = -2\left(1 - R\frac{\alpha}{y}\right),

.. math::

   N_r = 2R\alpha^2\beta\frac{k^2}{xyz}.

For :math:`j \in \{u,v,s,r\}` define

.. math::

   q_j = \frac{1}{2} a_j(a_j S - 2\Delta),

.. math::

   y_j = \frac{a_j S - \Delta}{\sqrt{2S}},

.. math::

   z_j = -a_j\Delta + i\frac{1}{2}a_j H.

Now define the Gaussian-like and Lorentzian-like building blocks

.. math::

   K_j = e^{q_j} \operatorname{erfc}(y_j),

.. math::

   F_j = e^{z_j}E_1(z_j), \qquad J_j = \Im(F_j),

Where :math:`\Im` denotes taking the imaginary component.

and the two sums

.. math::

   G = N_u K_u + N_v K_v + N_s K_s + N_r K_r,

.. math::

   L = N_u J_u + N_v J_v + N_s J_s + N_r J_r.

The implemented function is therefore

.. math::

   f = I Q \left[(1-\eta)G - \frac{2\eta}{\pi}L\right].

Basic derivatives of the full function
--------------------------------------

Let

.. math::

   M = (1-\eta)G - \frac{2\eta}{\pi}L,

so that :math:`f = IQM`.

The derivative with respect to intensity is immediate:

.. math::

   \frac{\partial f}{\partial I} = QM.

For :math:`p = \eta`, the prefactor :math:`Q` does not depend on
:math:`\eta`, so

.. math::

   \frac{\partial f}{\partial \eta}
   = IQ\frac{\partial M}{\partial \eta}
   = -IQ\left(G + \frac{2}{\pi}L\right).

For any other natural parameter :math:`p`,

.. math::

   \frac{\partial f}{\partial p}
   = I\left[\frac{\partial Q}{\partial p} M + Q\frac{\partial M}{\partial p}\right],

with

.. math::

   \frac{\partial M}{\partial p}
   = (1-\eta)\frac{\partial G}{\partial p}
     - \frac{2\eta}{\pi}\frac{\partial L}{\partial p}.

Since

.. math::

   Q = \frac{1}{4} \alpha \frac{1-k^2}{k^2},

it follows that

.. math::

   \frac{\partial Q}{\partial p}
   = \begin{cases}
     \dfrac{Q}{\alpha}, & p = \alpha, \\
     0, & p \neq \alpha.
     \end{cases}

Derivative of :math:`G`
-----------------------

The Gaussian-like sum is

.. math::

   G = \sum_{j \in \{u,v,s,r\}} N_j K_j,

so by the product rule

.. math::

   \frac{\partial G}{\partial p}
   = \sum_{j \in \{u,v,s,r\}}
     \left[ \frac{\partial N_j}{\partial p}K_j + N_j\frac{\partial K_j}{\partial p} \right].

The derivative of :math:`K_j`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Start from

.. math::

   K_j = e^{q_j}\operatorname{erfc}(y_j).

Differentiate directly:

.. math::

   \frac{\partial K_j}{\partial p}
   = \frac{\partial e^{q_j}}{\partial p}\operatorname{erfc}(y_j)
     + e^{q_j}\frac{\partial\operatorname{erfc}(y_j)}{\partial p}.

Using

.. math::

   \frac{\partial e^{q_j}}{\partial p} = \frac{\partial q_j}{\partial p}e^{q_j},

and

.. math::

   \frac{d}{dy}\operatorname{erfc}(y) = -\frac{2}{\sqrt{\pi}}e^{-y^2},

we obtain

.. math::

   \frac{\partial K_j}{\partial p}
   = \frac{\partial q_j}{\partial p}K_j
     - \frac{2}{\sqrt{\pi}} e^{q_j-y_j^2} \frac{\partial y_j}{\partial p}.

Now simplify :math:`q_j - y_j^2`:

.. math::

   q_j = \frac{a_j^2S - 2a_j\Delta}{2},

.. math::

   y_j^2 = \frac{(a_jS - \Delta)^2}{2S}
         = \frac{a_j^2S^2 - 2a_j\Delta S + \Delta^2}{2S}
         = \frac{a_j^2S - 2a_j\Delta + \Delta^2/S}{2},

so

.. math::

   q_j - y_j^2 = -\frac{\Delta^2}{2S}.

Therefore the exponential prefactor is independent of :math:`j`:

.. math::

   e^{q_j-y_j^2} = e^{-\Delta^2/(2S)}.

Define the common factor

.. math::

   C_0 = \frac{2}{\sqrt{\pi}} e^{-\Delta^2/(2S)}.

Then

.. math::

   \frac{\partial K_j}{\partial p}
   = \frac{\partial q_j}{\partial p}K_j - C_0\frac{\partial y_j}{\partial p}.

Useful special cases are derived below.

Derivative of :math:`K_j` with respect to :math:`a_j`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

From

.. math::

   q_j = \frac{1}{2}a_j^2S - \Delta a_j,

we have

.. math::

   \frac{\partial q_j}{\partial a_j} = a_j S - \Delta.

Also,

.. math::

   y_j = \frac{a_jS - \Delta}{\sqrt{2S}},

so

.. math::

   \frac{\partial y_j}{\partial a_j} = \frac{S}{\sqrt{2S}} = \sqrt{\frac{S}{2}}.

Hence

.. math::

   \frac{\partial K_j}{\partial a_j}
   = (a_jS - \Delta)K_j - C_0\sqrt{\frac{S}{2}}.

Derivative of :math:`K_j` with respect to :math:`S`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since

.. math::

   q_j = \frac{1}{2}a_j(a_jS - 2\Delta),

we have

.. math::

   \frac{\partial q_j}{\partial S} = \frac{1}{2}a_j^2.

For :math:`y_j`, write

.. math::

   y_j = \frac{a_jS - \Delta}{\sqrt{2S}}.

Differentiating gives

.. math::

   \frac{\partial y_j}{\partial S}
   = \frac{a_j}{\sqrt{2S}} + (a_jS - \Delta)\frac{\partial}{\partial S}(2S)^{-1/2}
   = \frac{a_j}{\sqrt{2S}} - \frac{a_jS - \Delta}{(2S)^{3/2}}.

This simplifies to

.. math::

   \frac{\partial y_j}{\partial S}
   = \frac{a_jS + \Delta}{2S\sqrt{2S}}.

Therefore

.. math::

   \frac{\partial K_j}{\partial S}
   = \frac{1}{2}a_j^2 K_j - C_0\left[\frac{a_jS + \Delta}{2S\sqrt{2S}}\right].

Derivative of :math:`K_j` with respect to :math:`\Delta`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

From the same definitions,

.. math::

   \frac{\partial q_j}{\partial \Delta} = -a_j,

.. math::

   \frac{\partial y_j}{\partial \Delta} = -\frac{1}{\sqrt{2S}}.

Hence

.. math::

   \frac{\partial K_j}{\partial \Delta}
   = -a_jK_j + \frac{C_0}{\sqrt{2S}}.

Derivative of :math:`L`
-----------------------

The Lorentzian-like sum is

.. math::

   L = \sum_{j \in \{u,v,s,r\}} N_j J_j,

so similarly

.. math::

   \frac{\partial L}{\partial p}
   = \sum_{j \in \{u,v,s,r\}}
     \left[ \frac{\partial N_j}{\partial p}J_j + N_j\frac{\partial J_j}{\partial p} \right].

The derivative of :math:`J_j`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Recall that

.. math::

   J_j = \Im(F_j), \qquad F_j = e^{z_j}E_1(z_j).

Use the identity

.. math::

   \frac{d}{dz}\left(e^z E_1(z)\right) = e^zE_1(z) - \frac{1}{z},

so

.. math::

   \frac{\partial F_j}{\partial p}
   = \left(F_j - \frac{1}{z_j}\right)\frac{\partial z_j}{\partial p}.

Taking imaginary parts gives

.. math::

   \frac{\partial J_j}{\partial p}
   = \Im\left[\left(F_j - \frac{1}{z_j}\right)\frac{\partial z_j}{\partial p}\right].

Useful special cases are obtained from

.. math::

   z_j = -a_j\Delta + i\frac{1}{2}a_jH.

Derivative of :math:`J_j` with respect to :math:`a_j`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since

.. math::

   \frac{\partial z_j}{\partial a_j} = -\Delta + i\frac{H}{2},

we have

.. math::

   \frac{\partial J_j}{\partial a_j}
   = \Im\left[\left(F_j - \frac{1}{z_j}\right)\left(-\Delta + i\frac{H}{2}\right)\right].

Derivative of :math:`J_j` with respect to :math:`H`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Now

.. math::

   \frac{\partial z_j}{\partial H} = i\frac{a_j}{2},

so

.. math::

   \frac{\partial J_j}{\partial H}
   = \Im\left[\left(F_j - \frac{1}{z_j}\right)i\frac{a_j}{2}\right]
   = \frac{a_j}{2}\Re\left(F_j - \frac{1}{z_j}\right).

Where :math:`\Re` denotes taking the real component

Derivative of :math:`J_j` with respect to :math:`\Delta`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Finally,

.. math::

   \frac{\partial z_j}{\partial \Delta} = -a_j,

hence

.. math::

   \frac{\partial J_j}{\partial \Delta}
   = -a_j\Im\left(F_j - \frac{1}{z_j}\right).

Derivatives of the branch coefficients
--------------------------------------

Derivatives with respect to :math:`R`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These follow immediately:

.. math::

   \frac{\partial N_u}{\partial R} = -\frac{a_u}{x},

.. math::

   \frac{\partial N_v}{\partial R} = -\frac{a_v}{z},

.. math::

   \frac{\partial N_s}{\partial R} = 2\frac{\alpha}{y},

.. math::

   \frac{\partial N_r}{\partial R} = 2\alpha^2\beta\frac{k^2}{xyz}.

Derivatives with respect to :math:`\alpha`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For :math:`N_u`, write

.. math::

   N_u = 1 - R\frac{a_u}{x}, \qquad a_u = \alpha(1-k), \qquad x = a_u - \beta.

Then

.. math::

   \frac{\partial N_u}{\partial \alpha}
   = -R\frac{\partial}{\partial \alpha}\left(\frac{a_u}{x}\right)
   = -R\frac{(\partial a_u/\partial \alpha)x - a_u(\partial x/\partial \alpha)}{x^2}.

Since

.. math::

   \frac{\partial a_u}{\partial \alpha} = 1-k, \qquad \frac{\partial x}{\partial \alpha} = 1-k,

we obtain

.. math::

   \frac{\partial N_u}{\partial \alpha}
   = -R(1-k)\frac{x-a_u}{x^2}
   = R(1-k)\frac{\beta}{x^2}.

Similarly,

.. math::

   \frac{\partial N_v}{\partial \alpha} = R(1+k)\frac{\beta}{z^2}.

For :math:`N_s = -2(1 - R\alpha/y)`,

.. math::

   \frac{\partial N_s}{\partial \alpha}
   = 2R\frac{\partial}{\partial \alpha}\left(\frac{\alpha}{y}\right)
   = 2R\frac{y-\alpha}{y^2}
   = -2R\frac{\beta}{y^2}.

For :math:`N_r = 2R\alpha^2\beta k^2/(xyz)`, use the logarithmic derivative:

.. math::

   \frac{1}{N_r}\frac{\partial N_r}{\partial \alpha}
   = \frac{2}{\alpha} - \frac{1}{x}\frac{\partial x}{\partial \alpha}
     - \frac{1}{y}\frac{\partial y}{\partial \alpha}
     - \frac{1}{z}\frac{\partial z}{\partial \alpha}.

Since

.. math::

   \frac{\partial x}{\partial \alpha} = 1-k, \qquad
   \frac{\partial y}{\partial \alpha} = 1, \qquad
   \frac{\partial z}{\partial \alpha} = 1+k,

this becomes

.. math::

   \frac{\partial N_r}{\partial \alpha}
   = N_r\left[\frac{2}{\alpha} - \frac{1-k}{x} - \frac{1}{y} - \frac{1+k}{z}\right].

Derivatives with respect to :math:`\beta`
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

For :math:`N_u = 1 - Ra_u/x` and :math:`\partial x/\partial \beta = -1`,

.. math::

   \frac{\partial N_u}{\partial \beta}
   = -\frac{Ra_u}{x^2}.

Similarly,

.. math::

   \frac{\partial N_v}{\partial \beta} = -\frac{Ra_v}{z^2},

.. math::

   \frac{\partial N_s}{\partial \beta} = 2R\frac{\alpha}{y^2}.

For :math:`N_r`, another logarithmic derivative gives

.. math::

   \frac{1}{N_r}\frac{\partial N_r}{\partial \beta}
   = \frac{1}{\beta} - \frac{1}{x}\frac{\partial x}{\partial \beta}
     - \frac{1}{y}\frac{\partial y}{\partial \beta}
     - \frac{1}{z}\frac{\partial z}{\partial \beta}.

Because

.. math::

   \frac{\partial x}{\partial \beta} = \frac{\partial y}{\partial \beta} = \frac{\partial z}{\partial \beta} = -1,

we obtain

.. math::

   \frac{\partial N_r}{\partial \beta}
   = N_r\left[\frac{1}{\beta} + \frac{1}{x} + \frac{1}{y} + \frac{1}{z}\right].

Using :math:`a_{j,p}` to lift branch derivatives to :math:`\alpha` and :math:`\beta`
--------------------------------------------------------------------------------------

The derivatives above for :math:`K_j` and :math:`J_j` were first written with
respect to the branch variable :math:`a_j`. These are converted to derivatives
with respect to :math:`\alpha` and :math:`\beta` by the chain rule.

Only the following branch derivatives are non-zero:

.. math::

   \frac{\partial a_u}{\partial \alpha} = 1-k, \qquad
   \frac{\partial a_v}{\partial \alpha} = 1+k, \qquad
   \frac{\partial a_s}{\partial \alpha} = 1, \qquad
   \frac{\partial a_r}{\partial \alpha} = 0,

and

.. math::

   \frac{\partial a_u}{\partial \beta} = 0, \qquad
   \frac{\partial a_v}{\partial \beta} = 0, \qquad
   \frac{\partial a_s}{\partial \beta} = 0, \qquad
   \frac{\partial a_r}{\partial \beta} = 1.

Therefore

.. math::

   \frac{\partial K_u}{\partial \alpha} = \frac{\partial K_u}{\partial a_u}(1-k),
   \qquad
   \frac{\partial K_v}{\partial \alpha} = \frac{\partial K_v}{\partial a_v}(1+k),
   \qquad
   \frac{\partial K_s}{\partial \alpha} = \frac{\partial K_s}{\partial a_s},
   \qquad
   \frac{\partial K_r}{\partial \alpha} = 0,

and similarly

.. math::

   \frac{\partial K_r}{\partial \beta} = \frac{\partial K_r}{\partial a_r},

with the other :math:`\partial K_j/\partial \beta` terms equal to zero.

Exactly the same pattern applies to :math:`J_j`.

Collecting the natural-parameter derivatives
--------------------------------------------

With the pieces above, the natural-parameter derivatives are

.. math::

   \frac{\partial G}{\partial p}
   = \sum_{j \in \{u,v,s,r\}}
     \left[ \frac{\partial N_j}{\partial p}K_j + N_j\frac{\partial K_j}{\partial p} \right],

.. math::

   \frac{\partial L}{\partial p}
   = \sum_{j \in \{u,v,s,r\}}
     \left[ \frac{\partial N_j}{\partial p}J_j + N_j\frac{\partial J_j}{\partial p} \right],

.. math::

   \frac{\partial M}{\partial p}
   = (1-\eta)\frac{\partial G}{\partial p}
     - \frac{2\eta}{\pi}\frac{\partial L}{\partial p},

.. math::

   \frac{\partial f}{\partial p}
   = I\left[\frac{\partial Q}{\partial p}M + Q\frac{\partial M}{\partial p}\right],

for :math:`p \neq \eta`, together with

.. math::

   \frac{\partial f}{\partial \eta}
   = -IQ\left(G + \frac{2}{\pi}L\right).

Chain rule back to Mantid parameters
------------------------------------

The derivatives with respect to the natural parameters now need to be mapped
back to the parameters exposed by Mantid.

Alpha0 and Alpha1
~~~~~~~~~~~~~~~~~

Since

.. math::

   \alpha = \frac{1}{A_0 + \lambda A_1},

we have

.. math::

   \frac{\partial \alpha}{\partial A_0} = -\alpha^2,

.. math::

   \frac{\partial \alpha}{\partial A_1} = -\lambda\alpha^2.

Therefore

.. math::

   \frac{\partial f}{\partial A_0} = -\alpha^2\frac{\partial f}{\partial \alpha},

.. math::

   \frac{\partial f}{\partial A_1} = -\lambda\alpha^2\frac{\partial f}{\partial \alpha}.

Beta0
~~~~~

Because

.. math::

   \beta = \frac{1}{B_0},

it follows that

.. math::

   \frac{\partial \beta}{\partial B_0} = -\beta^2,

so

.. math::

   \frac{\partial f}{\partial B_0} = -\beta^2\frac{\partial f}{\partial \beta}.

Kappa
~~~~~

With

.. math::

   R = \exp\left(-\frac{81.799}{\kappa\lambda^2}\right),

we obtain

.. math::

   \frac{\partial R}{\partial \kappa}
   = \frac{81.799}{\kappa^2\lambda^2}R.

Hence

.. math::

   \frac{\partial f}{\partial \kappa}
   = \frac{81.799}{\kappa^2\lambda^2}R\frac{\partial f}{\partial R}.

X0
~~

Since

.. math::

   \Delta = x_i - X_0,

we have

.. math::

   \frac{\partial \Delta}{\partial X_0} = -1,

and therefore

.. math::

   \frac{\partial f}{\partial X_0} = -\frac{\partial f}{\partial \Delta}.

SigmaSquared and Gamma
~~~~~~~~~~~~~~~~~~~~~~

The final two Mantid parameters are :math:`V = \mathrm{SigmaSquared}` and
:math:`\Gamma_v = \mathrm{Gamma}`. The code first converts these Voigt
parameters into pseudo-Voigt parameters :math:`H` and :math:`\eta`.

Define

.. math::

   g = \sqrt{8\ln 2\, V},

.. math::

   P = g^5 + 2.69269 g^4\Gamma_v + 2.42843 g^3\Gamma_v^2
       + 4.47163 g^2\Gamma_v^3 + 0.07842 g\Gamma_v^4 + \Gamma_v^5,

.. math::

   H = P^{1/5},

.. math::

   t = \frac{\Gamma_v}{H},

.. math::

   \eta = 1.36603 t - 0.47719 t^2 + 0.11116 t^3,

.. math::

   S = \frac{H^2}{8\ln 2}.

The derivative of :math:`H` with respect to :math:`P` is

.. math::

   \frac{\partial H}{\partial P}
   = \frac{1}{5}P^{-4/5}
   = \frac{1}{5H^4}.

Therefore

.. math::

   \frac{\partial H}{\partial V}
   = \frac{1}{5H^4}\frac{\partial P}{\partial V}
   = \frac{1}{5H^4}\frac{\partial P}{\partial g}\frac{\partial g}{\partial V},

and since

.. math::

   \frac{\partial g}{\partial V}
   = \frac{4\ln 2}{g},

we have

.. math::

   \frac{\partial H}{\partial V}
   = \frac{1}{5H^4}\frac{\partial P}{\partial g}\frac{4\ln 2}{g}.

Likewise,

.. math::

   \frac{\partial H}{\partial \Gamma_v}
   = \frac{1}{5H^4}\frac{\partial P}{\partial \Gamma_v}.

The explicit polynomial derivatives are

.. math::

   \frac{\partial P}{\partial g}
   = 5g^4 + 4(2.69269)g^3\Gamma_v + 3(2.42843)g^2\Gamma_v^2
     + 2(4.47163)g\Gamma_v^3 + 0.07842\Gamma_v^4,

.. math::

   \frac{\partial P}{\partial \Gamma_v}
   = 2.69269 g^4 + 2(2.42843)g^3\Gamma_v + 3(4.47163)g^2\Gamma_v^2
     + 4(0.07842)g\Gamma_v^3 + 5\Gamma_v^4.

Next,

.. math::

   \frac{\partial S}{\partial H} = \frac{H}{4\ln 2},

so

.. math::

   \frac{\partial S}{\partial V}
   = \frac{H}{4\ln 2}\frac{\partial H}{\partial V},

.. math::

   \frac{\partial S}{\partial \Gamma_v}
   = \frac{H}{4\ln 2}\frac{\partial H}{\partial \Gamma_v}.

For :math:`\eta`, use

.. math::

   \frac{\partial \eta}{\partial t}
   = 1.36603 - 0.95438 t + 0.33348 t^2.

Since :math:`t = \Gamma_v/H`,

.. math::

   \frac{\partial t}{\partial V}
   = -\frac{\Gamma_v}{H^2}\frac{\partial H}{\partial V},

.. math::

   \frac{\partial t}{\partial \Gamma_v}
   = \frac{1}{H} - \frac{\Gamma_v}{H^2}\frac{\partial H}{\partial \Gamma_v}.

Hence

.. math::

   \frac{\partial \eta}{\partial V}
   = \frac{\partial \eta}{\partial t}\frac{\partial t}{\partial V},

.. math::

   \frac{\partial \eta}{\partial \Gamma_v}
   = \frac{\partial \eta}{\partial t}\frac{\partial t}{\partial \Gamma_v}.

Finally, the Mantid-parameter derivatives are

.. math::

   \frac{\partial f}{\partial V}
   = \frac{\partial f}{\partial S}\frac{\partial S}{\partial V}
     + \frac{\partial f}{\partial H}\frac{\partial H}{\partial V}
     + \frac{\partial f}{\partial \eta}\frac{\partial \eta}{\partial V},

.. math::

   \frac{\partial f}{\partial \Gamma_v}
   = \frac{\partial f}{\partial S}\frac{\partial S}{\partial \Gamma_v}
     + \frac{\partial f}{\partial H}\frac{\partial H}{\partial \Gamma_v}
     + \frac{\partial f}{\partial \eta}\frac{\partial \eta}{\partial \Gamma_v}.

Summary
-------

The analytical derivative is most naturally implemented in two stages:

#. derive :math:`\partial f / \partial p` for the natural parameters
   :math:`(I, \alpha, \beta, R, S, H, \eta, \Delta)`;
#. use the chain rule to map these to the Mantid parameters
   :math:`(I, \mathrm{Alpha0}, \mathrm{Alpha1}, \mathrm{Beta0}, \mathrm{Kappa},
   \mathrm{SigmaSquared}, \mathrm{Gamma}, X_0)`.
