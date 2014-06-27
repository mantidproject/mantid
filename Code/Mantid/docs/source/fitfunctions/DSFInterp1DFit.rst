.. _func-DSFInterp1DFit:

.. index:: DSFInterp1DFit

==============
DSFInterp1DFit
==============


Description
----------- 

Given a set of parameter values :math:`T_i` and corresponding structure factors :math:`S(Q,E,T_i)`, this
fit function interpolates :math:`S(Q,E,T)` for any value of parameter T within the range spanned by the :math:`T_i` set in order to fit against a reference :math:`S(Q,E)`.

This fitting function is closely related to algorithm :ref:`DSFinterp <algm-DSFinterp>`. Please check the algorithm wiki page to learn about the details of the interpolator.

Atributes (non-fitting parameters)
----------------------------------

===== ================ ========= ========= ========= ===========================================================================
Order Name             Direction Type      Default   Description
===== ================ ========= ========= ========= ===========================================================================
1     InputWorkspaces  Input     str list  Mandatory list of input workspace names in a single string, separated by white spaces
2     LoadErrors       Input     bool      True      Do we load error data contained in the workspaces? 
3     ParameterValues  Input     dbl list  Mandatory list of input parameter values, as a single string separated by white spaces
4     LocalRegression  Input     bool      True      Perform running local-regression?
5     RegressionWindow Input     number    6         window size for the running local-regression
6     RegressionType   Input     str       quadratic type of local-regression; linear and quadratic are available
===== ================ ========= ========= ========= ===========================================================================


Fitting parameters
------------------

===== =============== ==== ======= ================================================================================
Order Name            Type Default Description
===== =============== ==== ======= ================================================================================
1     Intensity       dbl  1.0     Multiplicative prefactor scaling the height or intensity of the structure factor
2     TargetParameter dbl  1.0     Parameter value for which the interpolator is evaluated
===== =============== ==== ======= ================================================================================

.. categories:: FitFunctions QuasiElastic