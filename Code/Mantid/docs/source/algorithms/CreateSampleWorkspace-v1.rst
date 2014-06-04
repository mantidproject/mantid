.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates sample workspaces for usage examples and other situations.

You can select a predefined function for the data or enter your own by
selecting User Defined in the drop down.

The data will be the same for each spectrum, and is defined by the
function selected, and a little noise if Random is selected. All values
are taken converted to absolute values at present so negative values
will become positive. For event workspaces the intensity of the graph
will be affected by the number of events selected.

Here is an example of a user defined formula containing two peaks and a
background.

``name=LinearBackground, A0=0.5;name=Gaussian, PeakCentre=10000, Height=50, Sigma=0.5;name=Gaussian, PeakCentre=1000, Height=80, Sigma=0.5``

Random also affects the distribution of events within bins for event
workspaces. If Random is selected the results will differ between runs
of the algorithm and will not be comparable. If comparing the output is
important set Random to false or uncheck the box.

.. categories::
