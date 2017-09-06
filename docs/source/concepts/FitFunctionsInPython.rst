.. _FitFunctionsInPython:

Fit Functions In Python
=======================

Introduction
------------

Mantid enables Fit function objects to be produced in python. For example

.. code:: python

  g = gaussian()

will make ``g`` into a Gaussian function with default values and

.. code:: python

  g = Gaussian(Height=1, Sigma=0.1)
  
will make ``g`` into a Gaussian function with ``Height`` set to 1, ``Sigma`` set to 0.1 and ``PeakCentre`` set to default value.

One can also make function with attributes such as

.. code:: python

  p = Polynomial(n=3, A0=1, A1=2, A2=4, A3=3)
  
One can get and set function parameters and attributes, by array operations, for example:

.. code:: python

  gSigma = g["Sigma"]
  g["Height"] = 1.5
  
One can also get and set function parameters and attributes, by dot operations, for example:

.. code:: python

  gSigma = g.Sigma
  g.Height = 1.5
  
Composite Functions
-------------------
Composite functions can be created by means of the plus operation and 
individual functions can be accessed by array index operations. 
For example:

.. code:: python

   spectrum = LinearBackground() + Gaussian(PeakCentre=1) + Gaussian(PeakCentre=2)
   peak1 = spectrum[1]
   peak1['Sigma'] = 0.123
   spectrum[2] = Lorentzian(PeakCentre=2)

which sets ``Sigma`` of the second function (first Gaussian) to 0.123 and changes the third function to a Lorentzian.

Similarly product functions can be constructed using the multiplication operator.

.. code:: python

   p = ExpDecay() * Gaussian(Height=1,Sigma=0.2)

One can get and set the parameters of a composite function, by array operations, 
but not by dot operations because the parameter name already contains a dot, for example:

.. code:: python

   f1Sigma = spectrum["f1.Sigma"]
   spectrum["f1.Height"] = 1.5
  
One can add a function by the ``+-`` operator or remove be the ``del`` function.

.. code:: python

   spectrum += Lorentzian(PeakCentre=3)
   del spectrum[1]
   
Also available is the ``len`` function and iteration over the member functions:

.. code:: python

   n_peaks = len(spectrum)
   for func in spectrum: 
      print(func) 
      
The plus and times operators are associative and 
so may not preserve a composite function within a composite function as such,
but replace it with a list of its member functions.
Instead you may use:

.. code:: python

   spectrum = CompositeFunctionWrapper(LinearBackground(), Gaussian(PeakCentre=1), Gaussian(PeakCentre=2))
   P = ProductFunctionWrapper(ExpDecay(), Gaussian(Height=1,Sigma=0.2))
   
Multi-Domain Functions
----------------------
Multi-Domain functions can be constructed like this:

.. code:: python

    md_fun = MultiDomainFunction(Gaussian(PeakCentre=1, Sigma=0.1), Gaussian(PeakCentre=1, Sigma=0.2), ..., global=['Height'])



Setting Ties
------------

Setting Constraints
-------------------      

.. categories:: Concepts
