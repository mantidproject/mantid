
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm explores the surface of the :math:`\chi^{2}` around its minimum and estimates the standard deviations for the parameters.
The value of the output property is a base name for two output table workspaces: '<Output>_errors' and '<Output>_pdf'.
The former workspace contains parameter error estimates and the latter shows :math:`\chi^{2}`'s 1d slices along each parameter.

The procedure for calculating errors of parameters is described in Chapter 15 of Numerical recipes in C [1] and Chapter 9
of Statistical Data Analysis [2]. Here, we summarise the main results.

Consider the input dataset :math:`D_0`, with fit parameters :math:`\mathbf a_0`. Assuming Gaussian noise in the data, it is possible
to create :math:`n` artificial datasets :math:`D_j` for :math:`j=1,2,..,n`. If we were to run the fit on each dataset,
a set of parameters :math:`\mathbf a_j` would be obtained. The distribution of these parameters,
about our original parameter set :math:`\delta \mathbf a =  \mathbf a_j - \mathbf a_0` is described by the multivariate normal distribution,

.. math::
    P(\delta \mathbf a ) \propto exp (-\Delta \chi^2)

where :math:`\Delta \chi^2=\chi^2(\mathbf a_J) - \chi^2(\mathbf a_0)`, is the difference between the chi squared statistics obtained for each parameter set,

.. math::
    \Delta \chi^2 = \sum_{i \in D_0} \left ( \frac{y_i -f(x_i;\mathbf a_J)}{\sigma_i}\right)^2 - \left ( \frac{y_i -f(x_i;\mathbf a_0)}{\sigma_i}\right)^2

If we consider the variation of a single parameter :math:`a_k`, while the other parameters are the values that minimize :math:`\chi^2`,
the quantity :math:`\Delta \chi^2` will be distributed as a chi squared distributed, :math:`f_{\chi^2}(x; \nu)` with 1 degree of freedom,
:math:`\nu=1`. From this distribution, the probability of finding increases less than :math:`\Delta \chi^2` can be found from the integral,

.. math::
	p = P(X < \Delta \chi^2 ) = \int_0^{\Delta \chi^2} f_{\chi^2}(x; 1) dx

where :math:`p` is the desired confidence level. For instance, if we consider a confidence level of :math:`p=68.3\%` (1-sigma),
we find a critical value for the chi squared fluctuation of :math:`\Delta \chi^2 < 1`. From this desired confidence level,
we can then calculate the parameter variation, (:math:`\sigma_{left}`, :math:`\sigma_{right}`), that increases chi squared by 1
and obtain a :math:`68.3\%` confidence interval for the single parameter :math:`a_k`:

.. math::
    P( a_k - \sigma_{left} < a_k <  a_k + \sigma_{right}) = 68.3\%

This algorithm obtains the 1-sigma confidence level by varying a single parameter at a time and recording the extremums
of the values that increase :math:`\chi^2` by :math:`1`. The results are outputted in the table '<Output>_errors',
which reports the left and right 1-sigma errors. This procedure is repeated for 2-sigma and 3-sigma,
with the results displayed in the columns (2-sigma) and (3-sigma). An additional value is also reported, termed the quadratic error.
This is the 1-sigma error obtained from a Taylor expansion of the chi squared function about its minimum:

.. math::
	\chi^2(\mathbf a + \delta \mathbf a) = \chi^2(\mathbf a)_{min} + \delta \mathbf a^T  \mathbf C \delta \mathbf a

where :math:`\mathbf{C}` is the covariance matrix obtained from least squares fitting of the data, see the :ref:`Fitting <algm-Fit>` documentation for further details.
For a linear model (or approximately linear model) the quadratic error should be equal to the left and right errors, i.e it will be symmetric.
This can be seen in the following example

.. code-block:: python

    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np

    # create the data
    x = np.linspace(1, 10, 250)
    np.random.seed(0)
    y = 1 + 2.0*x + 0.4*np.random.randn(x.size)
    ws = CreateWorkspace(x, y)

    # run the fit
    func = "name=LinearBackground, A0=1, A1=2";
    func_ = FunctionFactory.Instance().createInitialized(func)

    fit_output = Fit(InputWorkspace=ws, Function=func_, Output="LinearFit")
    a0_fit = fit_output.OutputParameters.column(1)[0]
    a1_fit = fit_output.OutputParameters.column(1)[1]

    # explore the chi squared profile for the fit parameters
    fitted_func = "name=LinearBackground, A0={}, A1={}".format(a0_fit, a1_fit);
    ProfileChiSquared1D(fitted_func, ws, Output="LinearProfile")

    # print left and right errors of parameters
    # you should note that they are approx equal to the quadratic error for this linear model
    error_table = mtd["LinearProfile_errors"]
    lerror_a0 = error_table.column(3)[0]
    rerror_a0= error_table.column(4)[0]
    qerror_a0 = error_table.column(9)[0]
    print("1-sigma error bounds of A0 are {} and {}, with quadratic estimate {}".format(lerror_a0, rerror_a0, qerror_a0))

    lerror_a1 = error_table.column(3)[1]
    rerror_a1= error_table.column(4)[1]
    qerror_a1 = error_table.column(9)[1]
    print("1-sigma error bounds of A1 are {} and {}, with quadratic estimate {}".format(lerror_a1, rerror_a1, qerror_a1))


For a non-linear model, it's possible that the left and right variances will not be equal, leading to an asymmetric error.
This is shown in the example below:

.. code-block:: python

    # import mantid algorithms, numpy and matplotlib
    from mantid.simpleapi import *
    import matplotlib.pyplot as plt
    import numpy as np


    # create decaying exponential data
    x = np.linspace(1, 10, 250)
    np.random.seed(0)
    y = 3.0*np.exp(-x/2) + 0.1*np.random.randn(x.size)
    ws = CreateWorkspace(x, y)

    # run the fit
    func = "name=ExpDecay,Height=3.0, Lifetime=0.5";
    func_ = FunctionFactory.Instance().createInitialized(func)

    fit_output = Fit(InputWorkspace=ws, Function=func_, Output="ExpFit")
    height_fit = fit_output.OutputParameters.column(1)[0]
    lifetime_fit = fit_output.OutputParameters.column(1)[1]

    # explore the chi squared profile for the fit parameters
    fitted_func = "name=ExpDecay, Height={}, Lifetime={}".format(height_fit, lifetime_fit);
    ProfileChiSquared1D(fitted_func, ws, Output="ExpProfile")

    # print left and right errors of parameters
    # you should note that they differ from the quadratic errors
    error_table = mtd["ExpProfile_errors"]
    lerror_height = error_table.column(3)[0]
    rerror_height= error_table.column(4)[0]
    qerror_height = error_table.column(9)[0]
    print("1-sigma error bounds of Height are {} and {}, with quadratic estimate {}".format(lerror_height, rerror_height, qerror_height))

    lerror_lifetime = error_table.column(3)[1]
    rerror_lifetime= error_table.column(4)[1]
    qerror_lifetime = error_table.column(9)[1]
    print("1-sigma error bounds of Lifetime are {} and {}, with quadratic estimate {}".format(lerror_lifetime, rerror_lifetime, qerror_lifetime))


For each problem, the error table has the following columns:

======================    ==============
Column                    Description
======================    ==============
Parameter                 Parameter name
Value                     Parameter value passed with the Function property
Value at Min              The minimum point of the 1d slice of the :math:`\chi^{2}`. If the Function is at the minimum then
                          Value at Min should be equal to Value.
Left Error (1-sigma)      The negative deviation from the minimum point equivalent to :math:`1\sigma`. Estimated from analysis
                          of the surface.
Right Error (1-sigma)     The positive deviation from the minimum point equivalent to :math:`1\sigma`. Estimated from analysis
                          of the surface.
Left Error (2-sigma)      The negative deviation from the minimum point equivalent to :math:`2\sigma`. Estimated from analysis
                          of the surface.
Right Error (2-sigma)     The positive deviation from the minimum point equivalent to :math:`2\sigma`. Estimated from analysis
                          of the surface.
Left Error (3-sigma)      The negative deviation from the minimum point equivalent to :math:`3\sigma`. Estimated from analysis
                          of the surface.
Right Error (3-sigma)     The positive deviation from the minimum point equivalent to :math:`3\sigma`. Estimated from analysis
                          of the surface.
Quadratic Error           :math:`1\sigma` standard deviation in the quadratic approximation of the surface.
======================    ==============

This algorithm also reports a probability density function (PDF) for each parameter, stored in the '<Output>_pdf' table.
This PDF is found from Eq. (1), which relates the increase in chi squared, to the probability of the parameter variation.
The pdf table also contains slices of the :math:`\chi^{2}` along each parameter. It has 3 column per parameter. The first column of the 3
is the parameter values, the second has the :math:`\chi^{2}` and the third is the probability density function normalised to
have 1 at the maximum. Plotting the second column of each parameter will show the change in :math:`\chi^{2}` with respect to
the parameter value.

References
----------

[1] William H. Press, Saul A. Teukolsky, William T. Vetterling, and Brian P. Flannery. 1992.
Numerical recipes in C (2nd ed.): the art of scientific computing. Cambridge University Press, USA.

[2] G. Cowan, Statistical Data Analysis, Clarendon, Oxford, 1998


.. categories::

.. sourcelink::
