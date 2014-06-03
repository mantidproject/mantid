.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

*Logarithm* function calculates the logarithm of the data, held in a
workspace and tries to estimate the errors of this data, by calculating
logarithmic transformation of the errors. The errors are assumed to be
small and Gaussian so they are calculated on the basis of Tailor
decomposition e.g. if :math:`S` and :math:`Err` are the signal and
errors for the initial signal, the logarithm would provide
:math:`S_{ln}=ln(S)` and :math:`Err_{ln}=Err/S` accordingly. If the base
10 logarithm is used the errors are calculated as
:math:`Err_{log10}=0.434Err/S`

Some values in a workspace can normally be equal to zero. Logarithm is
not calculated for values which are less or equal to 0, but the value of
*Filler* is used instead. The errors for such cells set to zeros

When acting on an event workspace, the output will be a Workspace2D,
with the default binning from the original workspace.

.. categories::
