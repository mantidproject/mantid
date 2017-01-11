.. _func-FunctionQDepends:

================
FunctionQDepends
================

.. index:: FunctionQDepends

Description
-----------

This fitting function is the base class for all fitting functions that have:

- A range of energy transfers as their one-dimensional domain from which to take values
- The magnitude of the momemtum transfer, :math:`Q`, as one of their attributes.

Fitting functions for QENS data depending on :math:`Q` should derive from this class.

There are two ways to update attribute :math:`Q` in the fit funtion:

- The user inputs a particular value.
- The spectrum contains :math:`Q` in addition to the range of energy transfers.

Conflict may arise when both options are available. In that case, priority is given to the :math:`Q`-value contained
in the spectrum. Here are some user cases:

- User cannot override the value of attribute :math:`Q` if the spectrum contains a :math:`Q`-value.
- User can set or update the value of of attribute :math:`Q` is the spectrum does not contain a :math:`Q`-value.
- The value of attribute :math:`Q` will be updated everytime we pass a new spectrum containing a :math:`Q`-value.
- The value of attribute :math:`Q` will be erased if we pass a new spectrum not containing a :math:`Q`-value. In this case it is the responsability of the user to set the appropriate value.
