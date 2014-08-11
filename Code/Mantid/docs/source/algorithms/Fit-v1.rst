.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Additional properties for a 1D function and a MatrixWorkspace
#############################################################

If Function defines a one-dimensional function and InputWorkspace is a
:ref:`MatrixWorkspace <MatrixWorkspace>` the algorithm will have these
additional properties:

+------------------+-------------+-----------+-------------------------+---------------------------------------------------------------------+
| Name             | Direction   | Type      | Default                 | Description                                                         |
+==================+=============+===========+=========================+=====================================================================+
| WorkspaceIndex   | Input       | integer   | 0                       | The spectrum to fit, using the workspace numbering of the spectra   |
+------------------+-------------+-----------+-------------------------+---------------------------------------------------------------------+
| StartX           | Input       | double    | Start of the spectrum   | An X value in the first bin to be included in the fit               |
+------------------+-------------+-----------+-------------------------+---------------------------------------------------------------------+
| EndX             | Input       | double    | End of the spectrum     | An X value in the last bin to be included in the fit                |
+------------------+-------------+-----------+-------------------------+---------------------------------------------------------------------+

Overview
########

This is a generic algorithm for fitting data in a Workspace with a
function. The workspace must have the type supported by the algorithm.
Currently supported types are: :ref:`MatrixWorkspace <MatrixWorkspace>` for
fitting with a `IFunction1D <http://www.mantidproject.org/IFunction1D>`_ and
`IMDWorkspace <http://www.mantidproject.org/IMDWorkspace>`_ for fitting with
`IFunctionMD <http://www.mantidproject.org/IFunctionMD>`_. After Function and InputWorkspace
properties are set the algorithm may decide that it needs more
information from the caller to locate the fitting data. For example, if
a spectrum in a MatrixWorkspace is to be fit with a 1D function it will
need to know at least the index of that spectrum. To request this
information Fit dynamically creates relevant properties which the caller
can set. Note that the dynamic properties depend both on the workspace
and the function. For example, the data in a MatrixWorkspace can be fit
with a 2D function. In this case all spectra will be used in the fit and
no additional properties will be declared. The Function property must be
set before any other.

The function and the initial values for its parameters are set with the
Function property. A function can be simple or composite. A `simple
function <../fitfunctions/categories/Functions.html>`__ has a name registered with Mantid
framework. The Fit algorithm creates an instance of a function by this
name. A composite function is an arithmetic sum of two or more functions
(simple or composite). Each function has a number of named parameters,
the names are case sensitive. All function parameters will be used in
the fit unless some of them are tied. Parameters can be tied by setting
the Ties property. A tie is a mathematical expression which is used to
calculate the value of a (dependent) parameter. Only the parameter names
of the same function can be used as variables in this expression.

Using the Minimizer property, Fit can be set to use different algorithms
to perform the minimization. By default if the function's derivatives
can be evaluated then Fit uses the GSL Levenberg-Marquardt minimizer.

In Mantidplot this algorithm can be run from the `Fit Property
Browser <MantidPlot:_Data Analysis and Curve Fitting#Simple_Peak_Fitting_with_the_Fit_Wizard>`__
which allows all the settings to be specified via its graphical user
interface.

Setting a simple function
#########################

To use a simple function for a fit set its name and initial parameter
values using the Function property. This property is a comma separated
list of name=value pairs. The name of the first name=value pairs must be
"name" and it must be set equal to the name of one of a `simple
function <../fitfunctions/categories/Functions.html>`__. This name=value pair is followed
by name=value pairs specifying values for the parameters of this
function. If a parameter is not set in Function it will be given its
default value defined by the function. All names are case sensitive. For
example for fitting a Gaussian the Function property might look like
this:

``Function: "name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5"``

Some functions have attributes. An attribute is a non-fitting parameter
and can be of one of the following types: text string, integer, or
double. Attributes are set just like the parameters using name=value
pairs. For example:

``Function: "name=UserFunction, Formula=a+b*x, a=1, b=2"``

In this example Formula is the name of a string attribute which defines
an expression for the user UserFunction. The fitting parameters a and b
are created when the Formula attribute is set. It is important that
Formula is defined before initializing the parameters.

A list of the available simple functions can be found
`here <../fitfunctions/categories/Functions.html>`__.

Setting a composite function
############################

A composite function is a sum of simple functions. It does not have a
name. To define a composite function set a number of simple functions in
the Function property. Each simple function definition must be separated
by a semicolon ';'. For example fitting two Gaussians on a linear
background might look like this::

   Function: "name=LinearBackground, A0=0.3; 
              name=Gaussian, PeakCentre=4.6, Height=10, Sigma=0.5;
              name=Gaussian, PeakCentre=7.6, Height=8, Sigma=0.5"

Setting ties
############

Parameters can be tied to other parameters or to a constant. In this
case they do not take part in the fitting but are evaluated using the
tying expressions. Use Ties property to set any ties. In case of a
simple function the parameter names are used as variables in the tying
expressions. For example

``Ties: "a=2*b+1, c=2"``

This ties parameter "a" to parameter "b" and fixes "c" to the constant
2.

In case of a composite function the variable name must refer to both the
parameter name and the simple function it belongs to. It is done by
writing the variable name in the following format:

``f``\ \ ``.``\

The format consists of two parts separated by a period '.'. The first
part defines the function by its index in the composite function
(starting at 0). The index corresponds to the order in which the
functions are defined in the Function property. For example:

``Ties: "f1.Sigma=f0.Sigma,f2.Sigma=f0.Sigma"``

This ties parameter "Sigma" of functions 1 and 2 to the "Sigma" of
function 0. Of course all three functions must have a parameter called
"Sigma" for this to work. The last example can also be written

``Ties: "f1.Sigma=f2.Sigma=f0.Sigma"``

Setting constraints
###################

Parameters can be constrained to be above a lower boundary and/or below
an upper boundary. If a constraint is violated a penalty to the fit is
applied which should result the parameters satisfying the constraint.
The penalty applied is described in more detail
:ref:`here <FitConstraint>`. Use Constraints property to set any
constraints. In case of a simple function the parameter names are used
as variables in the constraint expressions. For example

``Constraints: "4.0 < c < 4.2"``

Constraint the parameter "c" to be with the range 4.0 to 4.2, whereas

``Constraints: "c > 4.0"``

means "c" is constrained to be above the lower value 4.0 and

``Constraints: "c < 4.2"``

means "c" is constrained to be below the upper value 4.2.

In case of a composite function the same notation is used for
constraints and for ties. For example

``Constraints: "f1.c < 4.2"``

constrain the parameter "c" of function 1.

Fitting to data in a MatrixWorkspace
####################################

The error values in the input workspace are used to weight the data in
the fit. Zero error values are not allowed and are replaced with ones.

Output
######

Setting the Output property defines the names of the two output
workspaces. One of them is a `TableWorkspace <http://www.mantidproject.org/TableWorkspace>`_ with
the fitted parameter values. The other is a
:ref:`Workspace2D <Workspace2D>` which compares the fit with the original
data. It has three spectra. The first (index 0) contains the original
data, the second one the data simulated with the fitting function and
the third spectrum is the difference between the first two. For example,
if the Output was set to "MyResults" the parameter TableWorkspace will
have name "MyResults\_Parameters" and the Workspace2D will be named
"MyResults\_Workspace". If the function's derivatives can be evaluated
an additional TableWorkspace is returned. When the Output is set to
"MyResults" this TableWorkspace will have the name
"MyResults\_NormalisedCovarianceMatrix" and it returns a calculated
correlation matrix. Denote this matrix C and its elements Cij then the
diagonal elements are listed as 1.0 and the off diagnonal elements as
percentages of correlation between parameter i and j equal to
100\*Cij/sqrt(Cii\*Cjj).

Examples
--------

This example shows a simple fit to a Gaussian function. The algorithm
properties are:

| ``InputWorkspace:  Test``
| ``WorkspaceIndex:  0``
| ``Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5``
| ``Output:          res``

.. figure:: /images/GaussianFit.jpg
   :alt: GaussianFit.jpg

   GaussianFit.jpg

--------------

The next example shows a fit of the same data but with a tie.

| ``InputWorkspace:  Test``
| ``WorkspaceIndex:  0``
| ``Function:        name=Gaussian, PeakCentre=4, Height=1.3, Sigma=0.5``
| ``Ties:            Sigma=Height/2``
| ``Output:          res``

.. figure:: /images/GaussianFit_Ties.jpg
   :alt: GaussianFit_Ties.jpg

   GaussianFit\_Ties.jpg

--------------

This example shows a fit of two overlapping Gaussians on a linear
background. Here we create a composite function with a LinearBackground
and two Gaussians:

| ``InputWorkspace:  Test``
| ``WorkspaceIndex:  0``
| ``Function:        name=LinearBackground,A0=1;``
| ``                 name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;``
| ``                 name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 ``
| ``Output:          res``

.. figure:: /images/Gaussian2Fit.jpg
   :alt: Gaussian2Fit.jpg

   Gaussian2Fit.jpg

--------------

This example repeats the previous one but with the Sigmas of the two
Gaussians tied:

| ``InputWorkspace:  Test``
| ``WorkspaceIndex:  0``
| ``Function:        name=LinearBackground,A0=1;``
| ``                 name=Gaussian,PeakCentre=4,Height=1.5, Sigma=0.5;``
| ``                 name=Gaussian,PeakCentre=6,Height=4, Sigma=0.5 ``
| ``Ties:            f2.Sigma = f1.Sigma``
| ``Output:          res``

.. figure:: /images/Gaussian2Fit_Ties.jpg
   :alt: Gaussian2Fit_Ties.jpg

   Gaussian2Fit\_Ties.jpg

Usage
-----

**Example - Fit a Gaussian to a peak in a spectrum:**

.. testcode:: ExFitPeak

   # create a workspace with a gaussian peak sitting on top of a linear (here flat) background
   ws = CreateSampleWorkspace(Function="User Defined", UserDefinedFunction="name=LinearBackground, \
      A0=0.3;name=Gaussian, PeakCentre=5, Height=10, Sigma=0.7", NumBanks=1, BankPixelWidth=1, XMin=0, XMax=10, BinWidth=0.1)

   # Setup the data to fit:
   workspaceIndex = 0  # the spectrum with which WorkspaceIndex to fit
   startX = 1      # specify fitting region
   endX = 9      #

   # Setup the model, here a Gaussian, to fit to data
   tryCentre = '4'   # A start guess on peak centre
   sigma = '1'          # A start guess on peak width
   height = '8'         # A start guess on peak height
   myFunc = 'name=Gaussian, Height='+height+', PeakCentre='+tryCentre+', Sigma='+sigma
   # here purposely haven't included a linear background which mean fit will not be spot on
   # to include a linear background uncomment the line below
   #myFunc = 'name=LinearBackground, A0=0.3;name=Gaussian, Height='+height+', PeakCentre='+tryCentre+', Sigma='+sigma

   # Do the fitting
   fitStatus, chiSq, covarianceTable, paramTable, fitWorkspace = Fit(InputWorkspace='ws', \
      WorkspaceIndex=0, StartX = startX, EndX=endX, Output='fit', Function=myFunc)

   print "The fit was: " + fitStatus
   print("chi-squared of fit is: %.2f" % chiSq)
   print("Fitted Height value is: %.2f" % paramTable.column(1)[0])
   print("Fitted centre value is: %.2f" % paramTable.column(1)[1])
   print("Fitted sigma value is: %.2f" % paramTable.column(1)[2])
   # fitWorkspace contains the data, the calculated and the difference patterns
   print "Number of spectra in fitWorkspace is: " +  str(fitWorkspace.getNumberHistograms())
   print("The 20th y-value of the calculated pattern: %.4f" % fitWorkspace.readY(1)[19])

Output:

.. testoutput:: ExFitPeak

   The fit was: success
   chi-squared of fit is: 0.14
   Fitted Height value is: 9.79
   Fitted centre value is: 5.05
   Fitted sigma value is: 0.77
   Number of spectra in fitWorkspace is: 3
   The 20th y-value of the calculated pattern: 0.2361


.. categories::
