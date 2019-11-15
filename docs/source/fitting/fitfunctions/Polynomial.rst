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

**Example - Fit a polynomial background excluding some peaks

.. testcode:: Ex

    x = np.linspace(-10, 10, 100)
    y = np.exp(-4*(x+3)**2) + np.exp(-4*(x-3)**2) + 0.1 - 0.001*x**2
    ws = CreateWorkspace(x, y)
    Fit("name=Polynomial,n=2", ws, Exclude=[-5, -1, 1, 5], Output='out')

.. attributes::
    n;Integer;-;The degree of polynomial

.. properties::

.. categories::

.. sourcelink::
