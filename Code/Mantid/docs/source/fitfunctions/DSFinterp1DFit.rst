.. _func-DSFinterp1DFit:

==============
DSFinterp1DFit
==============

.. index:: DSFinterp1DFit

Description
----------- 

Given a set of parameter values :math:`T_i` and corresponding structure factors :math:`S(Q,E,T_i)`, this
fit function interpolates :math:`S(Q,E,T)` for any value of parameter T within the range spanned by the :math:`T_i` set in order to fit against a reference :math:`S(Q,E)`.

This fitting function is closely related to algorithm :ref:`DSFinterp <algm-DSFinterp>`. Please check the algorithm wiki page to learn about the details of the interpolator.

.. note::

   This fit function requires dsfinterp (https://pypi.python.org/pypi/dsfinterp).


Attributes (non-fitting parameters)
-----------------------------------

.. TODO should be an "attributes" tag here

================ ========= ========= ===========================================================================
Name             Type      Default   Description
================ ========= ========= ===========================================================================
InputWorkspaces  str list  Mandatory list of input workspace names in a single string, separated by white spaces
LoadErrors       bool      True      Do we load error data contained in the workspaces? 
ParameterValues  dbl list  Mandatory list of input parameter values, as a single string separated by white spaces
LocalRegression  bool      True      Perform running local-regression?
RegressionWindow number    6         window size for the running local-regression
RegressionType   str       quadratic type of local-regression; linear and quadratic are available
================ ========= ========= ===========================================================================

.. properties::

.. categories::
