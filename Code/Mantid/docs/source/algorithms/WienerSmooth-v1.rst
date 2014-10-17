
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm smooths the data in a histogram using the Wiener filter method.

Usage
-----

**Example - Smooth noisy data**

.. testcode::

    import random
    import math

    # Define a function
    def fun(x):
        return math.sin(x) + x /10

    # Create a workspace with noisy data.

    # Size of the data set
    n = 200

    # Define the x-values
    interval = (-10.0,10.0)
    dx = (interval[1] - interval[0]) / (n - 1)
    x = [interval[0] + dx * i for i in range(n)]

    # Define the y-values as a function fun(x) + random noise
    noise = 0.1
    y = [fun(x[i]) +  random.normalvariate(0.0,noise) for i in range(n)]

    # Create a workspace
    ws = CreateWorkspace(x,y)

    # Smooth the data using the Wiener filter
    smooth = WienerSmooth(ws,0)

.. categories::

