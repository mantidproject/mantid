
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. math:: \Delta\theta_{div} = \frac{1}{2}
          \sqrt{\Delta(2\theta)^2 + \alpha_0
          + \frac{4\left(\beta_0^2 + \beta_1^2\right)}{\sin^2(2\theta)}}

TODO: Enter a full rst-markup description of your algorithm here.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - EstimateDivergence**

.. testcode:: EstimateDivergenceExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = EstimateDivergence()

   # Print the result
   print "The output workspace has %%i spectra" %% wsOut.getNumberHistograms()

Output:

.. testoutput:: EstimateDivergenceExample

  The output workspace has ?? spectra

References
----------

#. Windsor, C. G. *Pulsed Neutron Scattering.* London: Taylor & Francis, 1981. Print. ISBN-10: 0470271310, ISBN-13: 978-0470271315

.. seealso :: Algorithm :ref:`algm-EstimateResolutionDiffraction`

.. categories::

.. sourcelink::
