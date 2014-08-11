.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Usage
^^^^^

DSFinterp(Workspaces, OutputWorkspaces, [LoadErrors], [ParameterValues], [LocalRegression], [RegressionWindow], [RegressionType], TargetParameters], [Version])
 
Required
^^^^^^^^

This algorithm requires python package `dsfinterp <https://github.com/camm-sns/dsfinterp>`_, available at the
`python package index <https://pypi.python.org/pypi/dsfinterp>`_.
If the package is not present, this algorithm will not be available. To install, type in a terminal 'sudo pip install dsfinterp'

Details
^^^^^^^

For every "dynamical channel" defined by one particular (Q,E) pair, the sequence of scalars 
{:math:`{S_i \equiv S(Q,E,T_i)}`} ordered by increasing value of T is interpolated
with a cubic spline, which then can be invoked to obtain
:math:`S(Q,E,T)` at any T value.

Errors in the structure factor are incorporated when constructing the spline, so that the spline
need not neccessarily pass trough the :math:`(T_i, S_i)` points.
This has the desirable effect of producing smooth spline curves when the variation of the
structure factors versus :math:`T` contains significant noise.
For more details on the construction of the spline, see `UnivariateSpline <http://docs.scipy.org/doc/scipy/reference/generated/scipy.interpolate.UnivariateSpline.html>`_

.. figure:: /images/DSFinterp_local_regression.png
   :alt: DSFinterp_local_regression.png
   :width: 600pt
   :height: 400pt
   :align: center

   Local quadratic regression of windowsize w=7 starting at index n=2
   
If the structure factors have no associated errors, an scenario typical of structure factors derived from simulations,
then error estimation can be implemented with the running, local regression option.
A local regression of windowsize :math:`w` starting at index :math:`n` performs a
linear squares minimization :math:`F` on the set of points :math:`(T_n,S_n),..,(T_{n+w},S_{n+w})`.
After the minimization is done, we record the expected value and error at :math:`T_{n+w/2}`:

value: :math:`S'_{n+w/2} = F(T_{n+w/2})`

error: :math:`e_{n+w/2} = \sqrt(\frac{1}{w}\sum_{j=n}^{n+w}(S_j-F(T_j))^2)`

As we slide the window along the T-axis, we obtain values and errors at every :math:`T_i`.
We use the {:math:`F(T_i)`} values and {:math:`e_i`} errors to produce a smooth spline,
as well as expected errors at any :math:`T` value.

Example
^^^^^^^

Our example system is a simulation of a small crystal of octa-methyl `silsesqioxane <http://www.en.wikipedia.org/wiki/Silsesquioxane>`_ molecules.
A total of 26 molecular dynamics simulations were performed under different values of the energy barrier
to methyl rotations, :math:`K`. Dynamics structure factors S(Q,E) were derived from each simulation.

.. figure:: /images/DSFinterp_fig3.png
   :alt: DSFinterp_fig3.png
   :width: 600pt
   :height: 400pt
   :align: center
   
   Interpolated spline (solid line) with associated errors at one (Q,E) dynamical channel. Red dots are values from the simulation used to construct the spline.
   
There are as many splines as dynamical channels. The algorithm gathers the interpolations
for each channel and aggregates them into an interpolated structure factor.

.. figure:: /images/DSFinterp_fig4.png
   :alt: DSFinterp_fig4.png
   :width: 600pt
   :height: 400pt
   :align: center
   
   Interpolated structure factor :math:`S(K,E|Q)`, in logarithm scaling, at fixed :math:`Q=0.9A^{-1}`.

.. categories::
