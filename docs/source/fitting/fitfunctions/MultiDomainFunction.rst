.. _func-MultiDomainFunction:

====================
MultiDomain Function
====================

.. index:: MultiDomainFunction

Description
-----------

A multidomain function includes information about
which domain a particular entry (function) will be applied to.
The members of the multidomain function are indexed from 0 to the number of functions minus 1.
Multidomain functions do not have their own parameters, instead they use
parameters of the member functions. Parameter names are formed from the
member function's index and its parameter name: f[domain_index].[name]. For
example, "f0.A0" would refer to the "A0" parameter of the first function in the multidomain function.

The input string to create a multidomain function is formed by including
the $domains=n directive in the function string. For example, the string for two
:ref:`Gaussians <func-Gaussian>` is given by,

``composite=MultiDomainFunction,NumDeriv=1;name=Gaussian,$domains=i,PeakCentre=0,Height=10,Sigma=0.7;name=Gaussian,$domains=i,PeakCentre=0.1,Height=10,Sigma=0.7``

where we note that the multidomain function is itself a :ref:`Composite function <func-CompositeFunction>`. The inclusion of $domains=i specifies that the ith
function will correspond to the ith domain of the data. As a concrete example, consider fitting two spectra (domains) of
a workspace,

.. code-block:: python

    def linear(x):
        return 1 + x

    def quadratic(x):
        return 1 + 3*x + 4*np.square(x)

    x = np.linspace(-2, 2, 151)
    y1 = linear(x)
    y2 = quadratic(x)
    ys = np.concatenate((y1, y2))

    ws = CreateWorkspace(DataX=x, DataY=ys, NSpec=2)

    multiFunc="composite=MultiDomainFunction,NumDeriv=1;name=Quadratic,$domains=1;name=LinearBackground,$domains=0"
    func = FunctionFactory.Instance().createInitialized(multiFunc)
    fit_output = Fit(Function=func,
                     InputWorkspace=ws, WorkspaceIndex=0,
                     InputWorkspace_1=ws, WorkspaceIndex_1=1,
                     Output='fit')

As $domains=1 is set for the Quadratic function, it will be used to fit the second domain of the input workspace,
corresponding to WorkspaceIndex=1. Similarly, the LinearBackground will be used to fit the first domain of the workspace,
WorkspaceIndex=0. If instead we had specified domains=i, the function would have been fit based on the order of creation,
i.e. the quadratic function would fit to the first domain and the LinearBackground to the second domain. If
the same function is to be applied to each domain the $domains=All$ option may be specified, for instance if we want to
fit a flat background across multiple domains,

.. code-block:: python

    from scipy import stats
    background = 2
    num_points = 200
    # generate random data from exponential distribution
    rv1 = stats.expon(loc=0, scale=2)
    x1 = rv1.rvs(size=num_points)
    x1.sort()
    y1 = rv1.pdf(x1) + background + 0.02*np.random.normal(0,1,num_points)

    rv2= stats.expon(loc=1, scale=2)
    x2 = rv2.rvs(size=num_points)
    x2.sort()
    y2 = rv2.pdf(x2) + background + 0.02*np.random.normal(0,1,num_points)

    ws = CreateWorkspace(DataX=np.concatenate((x1, x2)), DataY=np.concatenate((y1, y2)), NSpec=2)

    background=';name=FlatBackground,$domains=All'
    func=';name=ExpDecay,$domains=i'
    multiFunc='composite=MultiDomainFunction,NumDeriv=1' + func + func + background

    fit_output = Fit(Function=multiFunc,
                     InputWorkspace=ws, WorkspaceIndex=0,
                     InputWorkspace_1=ws, WorkspaceIndex_1=1,
                     Output='fit')

Each function of the multidomain function may itself be a composite function.
If a member function is a composite function the same principle applies, where 'f[domain_index].'
is prepended to each composite function. A multidomain function of composites may be generated as follows,

.. code-block:: python

    multidomain="composite=MultiDomainFunction,NumDeriv=true;"\
    "(composite=CompositeFunction,$domains=i;name=FlatBackground;name=Polynomial,n=2);"\
    "(composite=CompositeFunction,$domains=i;name=FlatBackground;name=Polynomial,n=2)"
    func = FunctionFactory.Instance().createInitialized(multidomain)

Which creates a multidomain function consisting of two composite functions, each composed of a FlatBackground and
second order polynomial.

Ties across domains can be added by appending a ties string to the multidomain function, for instance the following will create
a multidomain of FlatBackgrounds, where the A0 parameter is tied across both domains,

.. code-block:: python

    multidomain = 'composite=MultiDomainFunction,NumDeriv=1;name=FlatBackground,$domains=i,A0=0;name=FlatBackground,$domains=i,A0=0;ties=(f0.A0=f1.A0)'
    func = FunctionFactory.Instance().createInitialized(multidomain)

.. attributes::

.. properties::

.. categories::

.. sourcelink::
    :cpp: Framework/API/src/MultiDomainFunction.cpp
