.. _FitFunctionsInPython:

Fit Functions In Python
=======================

Introduction
------------

Mantid enables Fit function objects to be produced in python. For example

.. code:: python

  g = Gaussian()

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

    md_fun = MultiDomainFunction(Gaussian(PeakCentre=1, Sigma=0.1), Gaussian(PeakCentre=1, Sigma=0.2), ..., Global=['Height'])

Setting Ties
------------
The parameters of functions can be tied or fixed like this:

.. code:: python

    func1.tie(A0=2.0)
    func2.tie({'f1.A2': '2*f0.A1', 'f2.A2': '3*f0.A1 + 1'})
    func3.fix('A0')
    func4.fix('f2.A2')

Both fixes and ties can be removed by ``untie``:

.. code:: python

    func.untie('f3.Sigma')
    
To tie all parameters of the same local name in a composite function, one can use ``TieAll``:

.. code:: python

    func.tieAll('Sigma')
 
All members of the composite function must have this parameter (in this case ``Sigma``).
Similarly with fixing:

.. code:: python

    spectrum1.fixAll('FWHM')
    
Also parameters of a function can be fixed with ``fixAllParameters`` and unfixed with ``untieAllParameters``.

.. code:: python

    c.fixAllParameters()
    ... 
    c.untieAllParameters()


Setting Constraints
------------------- 
One can set and remove constraints as follows:

.. code:: python

    g.constrain("Sigma < 2.0, Height > 7.0") 
    ...
    g.unconstrain("Sigma")
    g.unconstrain("Height")
            
    comp.constrain("f1.Sigma < 2, f0.Height > 7") 
    ...
    comp.unconstrain("f1.Sigma")
    comp.unconstrain("f0.Height") 

One can all constrain a given parameter in all members of a composite function that have this parameter 
and also remove such constraints.

.. code:: python

    comp.constrainAll("Sigma < 1.8")
    ...
    comp.unconstrainAll("Sigma")
    
Evaluation
----------
One can evaluate functions:

.. code:: python

    p = Polynomial(n=2, A0=0, A1=0.5, A2=0.5)

    print p(1)  # expect 1.0
    print p(2)  # expect 3.0 
    print p(3)  # expect 6.0
    print p([0,1,2,3])  # expect [ 0. 1. 3. 6.]

    ws = CreateWorkspace(DataX=[0,1,2,3,4,5,6,7], DataY=[5,5,5,5,5,5,5])

    print p(ws).readY(1) # expect [  0.375   1.875   4.375   7.875  12.375  17.875  24.375]
    
One can use numpy arrays:

.. code:: python

    import numpy as np

    a = np.array([[0, 1,], [2, 3]])
    p = Polynomial(n=4, A0=1, A1=1, A2=1, A3=1, A4=1)
    print p(a)
    # expect [[   1.    5.]
    #         [  31.  121.]]

Also one can put parameters into the function when evaluating.

.. code:: python

   p = Polynomial(n=2)
   print p([0,1,2,3], 0.0, 0.5, 0.5) #expect [ 0. 1. 3. 6.]

This enables one to fit the functions with ``scipy.optimize.curve_fit``.  

Errors
------

The errors assoicated with a given parameter can be accessed using the ``getError`` method.
``getError`` takes either the parameter name or index as input. For example to get the error
on ``A1`` in the above polynomial, the code is:

.. code:: python

    # Find the parameter error by index
    error_A1 = p.getError(1)
    # Find the parameter error by name
    error_A1 = p.getError('A1')


Plotting
--------
Functions may be plotted by calling the ``plot`` method of the function.
``mantidplot`` must be available to import for ``plot`` to work.

This method can be called in any of the following manners:

.. code:: python

   f.plot(xValues=[0,2,2.5,3,5]) # for these x-values
   f.plot(workspace=ws) # for the x-values of workspace ws
   f.plot(workspace=ws, workspaceIndex=i)   # for x-values of workspace index i of ws
   f.plot(startX=xmin, endX=xmax)  # for 20 x-values between xmin and xmax
   f.plot(startX=xmin, endX=xmax, nSteps=10) # for 10 x-values between xmin and xmax
   f.plot(workspace=ws, startX=xmin, endX=xmax) # for x-values of ws between xmin & xmax
   f.plot(workspace=ws, name='Fred') # for plot & its workspace to be called 'Fred'  

If you use ``xValues``, then the list of x values must be in numerical order. 
This is not checked and if they are not in order the plot may fail to display properly.

If you want to display multiple plots of the same function, then use
``name`` to give each plot a unique name. The default value of ``name`` 
is the name of the function. 

.. categories:: Concepts
