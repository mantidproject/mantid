.. _Error Propagation:

Error_Propagation
=================

The purpose of this document is to explain how Mantid deals with Error
Propogation and how it is used in its algorithms.

Theory
------

In order to deal with error propagation, Mantid treats errors as a
guassian curve (also known as a bell curve or normal curve). Meaning
that if X = 100 +- 1 then it is still possible for a value of 102 to
occur, but far less likely than 101 or 99, then a value of 105 is far
less likely still than 102, and then 110 is simply unheard of.

This allows Mantid to work with the errors quite simply.

Plus and Minus Algorithm
------------------------

The plus algorithm adds a selection of datasets together, including
their margin of errors. Mantid has to therefore adapt the margin of
error so it continues to work with just one margin of error. The way it
does this is by simply adding together the certain values, for this
example we will use X\ :sub:`1` and X\ :sub:`2`. X\ :sub:`1` = 101 ± 2
and X\ :sub:`2` = 99 ± 2, Just to make it easier. Mantid takes the
average of the two definite values, 101 and 99.

X = 200 = (101 + 99).

The average of the error is calculated by taking the root of the sum of
the squares of the two error margins:

(√2:sup:`2` + 2\ :sup:`2`) = √8

X = 200 ± √8

Mantid deals with the minus algorithm similarly, doing the inverse
function of Plus.

Multiply and Divide Algorithm
-----------------------------

The Multiply and Divide Algorithm work slightly different from the Plus
and Minus Algorithms, in the sense that they have to be more complex.

To calculate error propagation, of say X\ :sub:`1` and X\ :sub:`2`.
X\ :sub:`1` = 101 ± 2 and X\ :sub:`2` = 99 ± 2 again, Mantid would
undertake the following calculation for divide:

Q = X\ :sub:`1`/X:sub:`2` = 101/99

Error Propogation = (√ ± 2/99 + ±2/101) All multiplied by Q = 0.22425

For the multiply algorithm, the only difference is in how Q is created,
which in turn affects the Error Propogation,

Q = X\ :sub:`1`\ \*X\ :sub:`2` = 101\*99

Error Propogation = (√ ± 2/99 + ±2/101) All multiplied by Q = 0.22425



.. categories:: Concepts