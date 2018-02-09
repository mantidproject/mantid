.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a wavelength normalization for SANS TOF instruments.

It uses the input workspaces group of several wavelengths and finds the K and B so

:math:`I_i(Scaled) = K_i * I_i(Original) - B_i`

For every workspace in the workspace group.

The output workspaces have the following suffixes:

- `_q_range_fit`: Workspace group with the range used for fitting. The fitting to find `K` and `B` is performed within this range.
- `_table`: Table with all the found parameters for every input workspace
- `trimmed_fit`: The Raw data with `K` and `B` applied.
- `trimmed_fit_averaged`:  A single `I(q)` with the raw data (with `K` and `B` applied) averaged.

.. categories::

.. sourcelink::
