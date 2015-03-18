.. _Error Propagation:

Error Propagation
=================

The purpose of this document is to explain how Mantid deals with Error
Propogation and how it is used in its algorithms.

Theory
------

In order to deal with error propagation, Mantid treats errors as guassian 
probabilities (also known as a bell curve or normal probabilities) and each 
observation as independent. Meaning that if X = 100 +- 1 then it is still 
possible for a value of 102 to occur, but less likely than 101 or 99, and a 
value of 105 is far less likely still than any of these values.

Plus and Minus Algorithm
------------------------

The plus algorithm adds a selection of datasets together, including their 
margin of errors. Mantid has to therefore adapt the margin of error so it 
continues to work with just one margin of error. The way it does this is by 
simply adding together the certain values. Consider the example where: 
X\ :sub:`1` = 101 ± 2 and X\ :sub:`2` = 99 ± 2. Then for the Plus algorithm

X = 200 = (101 + 99).

The propagated error is calculated by taking the root of the sum of the 
squares of the two error margins:

(√2:sup:`2` + 2\ :sup:`2`) = √8

Hence the result of the Plus algorithm can be summarised as:

X = 200 ± √8

Mantid deals with the Minus algorithm similarly.

Multiply and Divide Algorithm
-----------------------------

The Multiply and Divide Algorithm work slightly different from the Plus
and Minus Algorithms, in the sense that they have to be more complex, 
see also `here <http://en.wikipedia.org/wiki/Propagation_of_uncertainty>`_.

To calculate error propagation, of say X\ :sub:`1` and X\ :sub:`2`.
X\ :sub:`1` = 101 ± 2 and X\ :sub:`2` = 99 ± 2 ,Mantid would
undertake the following calculation for divide:

Q = X\ :sub:`1`/X:sub:`2` = 101/99

Error Propogation = (√ ± 2/99 + ±2/101) All multiplied by Q = 0.22425

For the multiply algorithm, the only difference is in how Q is created,
which in turn affects the Error Propogation,

Q = X\ :sub:`1`\ \*X\ :sub:`2` = 101\*99

Error Propogation = (√ ± 2/99 + ±2/101) All multiplied by Q = 0.22425



.. categories:: Concepts