
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

After fitting a model to POLDI 2D-data the residuals have to be analysed in order to determine whether the data are described by the model. Since this is very hard to see in the two dimensional residual data, the analysis involves a procedure that is very similar to the correlation method implemented in :ref:`algm-PoldiAutoCorrelation`. The method is slightly different because the residual at a given point may be either positive or negative.

The algorithm iteratively calculates the correlation spectrum of the residual data, distributes the correlation counts over the 2D residual data and normalizes the residuals so that their sum is equal to zero. The correlation spectra of all steps are accumulated and returned as output. In the spectrum it's for example possible to spot additional peaks, which may have been hidden in by larger peaks in a first data analysis.

.. categories::

