.. _plotting:

.. contents:: Table of contents
    :local:

============
Introduction
============

links to matplotlib and to mantid.plots

============
Simple plots
============

.. plot::
   :include-source:
   
   import matplotlib.pyplot as plt
   import numpy as np
   x = np.random.randn(1000)
   plt.hist( x, 20)
   plt.grid()
   plt.title(r'Normal: $\mu=%.2f, \sigma=%.2f$'%(x.mean(), x.std()))
   plt.show()
