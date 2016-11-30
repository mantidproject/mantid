.. _func-CompositeFunction:

=================
CompositeFunction
=================

.. index:: CompositeFunction

Description
-----------

A composite function is a function containing other functions. It
combines the values calculated by the member functions by adding them.
The members are indexed from 0 to the number of functions minus 1. The
indices are defined by the order in which the functions were added.
Composite functions do not have their own parameters, instead they use
parameters of the member functions. Parameter names are formed from the
member function's index and its parameter name: f[index].[name]. For
example, name "f0.Sigma" would be given to the "Sigma" parameter of a
Gaussian added first to the composite function. If a member function is
a composite function itself the same principle applies: 'f[index].' is
prepended to a name, e.g. "f0.f1.Sigma".

The input string to the Fit algorithm for a CompositeFunction is
constructed by joining the inputs of the member functions using the
semicolon ';' as a separator. For example, the string for two
:ref:`Gaussians <func-Gaussian>` with tied sigma parameters may look like the
following:

``name=Gaussian,PeakCentre=0,Height=1,Sigma=0.1,constraints=(0<Sigma<1);name=Gaussian,PeakCentre=1,Height=1,Sigma=0.1;ties=(f1.Sigma=f0.Sigma)``

Note that the ties clause is also separated by a semicolon. It is done
because the parameters are from different functions. Ties between
parameters of the same function can be placed inside the member
definition in which the local parameter names must be used, for example:

``name = FunctionType, P1=0, ties=( P2 = 2*P1 ); name = FunctionType, P1=0, ties=( P2 = 3 )``

which is equivalent to

``name = FunctionType, P1=0; name = FunctionType, P1=0; ties=( f0.P2 = 2*f0.P1, f1.P2 = 3 )``

Boundary constraints usually make sense to put in a local function
definition but they can also be moved to the composite function level,
i.e. can be separated by ';'. In this case the full parameter name must
be used, for example:

``name=Gaussian,PeakCentre=0,Height=1,Sigma=0.1;name=Gaussian,PeakCentre=1,Height=1,Sigma=0.1;ties=(f1.Sigma=f0.Sigma);constraints=(0<f0.Sigma<1)``

Mantid defines a number of fitting functions which extend
CompositeFunction. These are functions which also include other
functions but use different operations to combine them. Examples are
:ref:`ProductFunction <func-ProductFunction>` and :ref:`Convolution <func-Convolution>`.
Everything said about parameters of the CompositeFunction applies to
these functions.

Input strings of an extended composite function must start with
"composite=FunctionName;" and followed by the definitions of its members
as described for CompositeFunction. For example,

``composite=ProductFunction;name=LinearBackground;name=ExpDecay``

To define a composite function inside a composite function enclose the
inner one in brackets:

``name=LinearBackground;(composite=Convolution;name=Resolution;name=Lorentzian)``

.. attributes::

.. properties::

.. categories::

.. sourcelink::
    :cpp: Framework/API/src/CompositeFunction.cpp
