
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates a measure of the goodness of a fit in four ways.

The ChiSquared property returns the sum of squares of differences between the calculated and measured values:

:math:`\chi_{1}^{2} = \sum_{i} (y_i - f_i)^2`

where :math:`y_i` and :math:`f_i` are the measured and calculated values at i-th point.

The ChiSquaredDividedByDOF is ChiSquared divided by the number of degrees of freedom (DOF):

:math:`\chi_{2}^{2} = \frac{1}{DOF}\sum_{i} (y_i - f_i)^2`

:math:`DOF = N_d - N_p` where :math:`N_d` is the number of data points used in fitting and :math:`N_p`
is the number of free (not fixed or tied) parameters of the function.

The ChiSquaredWeighted property sums the squares of the differences divided by the data errors:

:math:`\chi_{3}^{2} = \sum_{i} \left(\frac{y_i - f_i}{\sigma_i}\right)^2`

Finally, ChiSquaredWeightedDividedByDOF is

:math:`\chi_{4}^{2} = \chi_{3}^{2} / DOF`

Usage
-----
**Example 1**

.. testcode:: CalculateChiSquaredExample

    import numpy as np

    # Create a data set
    x = np.linspace(0,1,10)
    y = 1.0 + 2.0 * x
    e = np.sqrt(y)
    ws = CreateWorkspace(DataX=x, DataY=y, DataE=e)

    # Define a function
    func = 'name=LinearBackground,A0=1.1,A1=1.9'

    # Calculate the chi squared
    chi2,chi2dof,chi2ndata,chi2W,chi2Wdof,chi2Wndata = CalculateChiSquared(func,ws)

    print('Chi squared is {:.13f}'.format(chi2))
    print('Chi squared / DOF is {:.14f}'.format(chi2dof))
    print('Chi squared / NDATA is {:.14f}'.format(chi2ndata))
    print('Chi squared weighted is {:.11f}'.format(chi2W))
    print('Chi squared weighted / DOF is {:.14f}'.format(chi2Wdof))
    print('Chi squared weighted / NDATA is {:.12f}'.format(chi2Wndata))

    # Define a function that models the data exactly
    func = 'name=LinearBackground,A0=1.0,A1=2.0'

    # Calculate the chi squared
    chi2,chi2dof,chi2ndata,chi2W,chi2Wdof,chi2Wndata = CalculateChiSquared(func,ws)

    print('Chi squared is {:.1f}'.format(chi2))
    print('Chi squared / DOF is {:.1f}'.format(chi2dof))
    print('Chi squared / NDATA is {:.1f}'.format(chi2ndata))
    print('Chi squared weighted is {:.1f}'.format(chi2W))
    print('Chi squared weighted / DOF is {:.1f}'.format(chi2Wdof))
    print('Chi squared weighted / NDATA is {:.1f}'.format(chi2Wndata))

Output:

.. testoutput:: CalculateChiSquaredExample

    Chi squared is 0.0351851851852
    Chi squared / DOF is 0.00439814814815
    Chi squared / NDATA is 0.00351851851852
    Chi squared weighted is 0.02660287840
    Chi squared weighted / DOF is 0.00332535979971
    Chi squared weighted / NDATA is 0.002660287840
    Chi squared is 0.0
    Chi squared / DOF is 0.0
    Chi squared / NDATA is 0.0
    Chi squared weighted is 0.0
    Chi squared weighted / DOF is 0.0
    Chi squared weighted / NDATA is 0.0

.. categories::

.. sourcelink::
