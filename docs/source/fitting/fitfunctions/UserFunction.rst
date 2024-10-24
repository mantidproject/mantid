.. _func-UserFunction:

============
UserFunction
============

.. index:: UserFunction

Description
-----------

A UserFunction is defined by a string formula. The formula is assigned
by setting string attribute Formula:

`` "name=UserFunction, Formula = h*sin(a*x), h=2, a=1"``

Formula must use 'x' for the x-values. The fitting parameters become
defined only after the Formula attribute is set that is why Formula must
go first in UserFunction definition.

.. attributes::

.. properties::

.. categories::

.. sourcelink::
    :h: Framework/CurveFitting/inc/MantidCurveFitting/Functions/UserFunction.h
    :cpp: Framework/CurveFitting/src/Functions/UserFunction.cpp
