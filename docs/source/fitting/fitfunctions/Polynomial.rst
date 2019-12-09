.. _func-Polynomial:

==========
Polynomial
==========

.. index:: Polynomial

Description
-----------

A polynomial function of degree :math:`n` is defined as:

.. math:: y = A_0 + A_1 \times x + A_2 \times x^2 + ... + A_n \times x^n,

where :math:`A_0, ...,  A_n` are constant coefficients.

Usage
-----

**Example - Fit a polynomial background excluding some peaks**

.. testcode:: Ex

    # create a sample workspace with a peak at x = 4 and a polynomial background
    x = np.linspace(0, 10, 100)
    y = np.exp(-4*(x-4)**2) -0.01*(x-5)**2 + 0.3
    ws = CreateWorkspace(x, y)
    
    # do a fit with the polynomial fit function
    Fit("name=Polynomial,n=2", ws, Exclude=[2, 6], Output='out')

.. attributes::
    n;Integer;-;The degree of polynomial

.. properties::

.. categories::

.. sourcelink::
