.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a polarised SANS scattering run and calculates the efficiency of the Helium analyzer at the time of the run as compared
to a reference time provided either as a time stamp in the ``ReferenceTimeStamp`` property or extracted from a ``ReferenceWorkspace``, which would typically
be an analyzer calibration run.

The efficiency is calculated from the following expression [#KRYCKA]_:

.. math::
    \epsilon_{cell} = \frac{ 1+ \tanh(\mu \, p_{He}(t_{run}, t_{ref}))}{2}

Where :math:`\mu` is the neutron attenuation length :

.. math::
    \mu = 0.0733 \, p \, d \, \lambda

And the polarization of the helium gas at the time of measurement, :math:`p_{He}(t_{run}, t_{ref})`, is calculated as follows:

.. math::
    p_{He}(t_{run}, t_{ref}) = p_{He_{0}} e^{-(| t_{run}- t_{ref} |)/\Gamma}

Input parameters are the pressure of the analyzer cell multiplied by cell length :math:`pd` (``PXD``), the initial polarization of Helium gas in the cell, :math:`p_{He_{0}}` (``InitialPolarization``), as
well as the lifetime of the polarized gas, :math:`\Gamma` (``Lifetime``) . Errors are calculated using standard error propagation considering no correlation between input parameters.

Optionally, the unpolarized transmission can be calculated if the  ``UnpolarizedTransmission`` parameter is set, following [#KRYCKA]_.

.. math::
    T_{{}^{3}He}^{unpol} = e^{-\mu} \cosh(\mu  p_{He})

Where we have taken the approximation that :math:`T_E = 1`.

The wavelength range and bins are extracted from the ``ReferenceWorkspace`` or else the ``InputWorkspace`` if a reference is not provided. These will be used to populated
the x axes of the ``OutputWorkspace``.

Usage
-----

**Example - Use case with default parameters:**

.. code:: python

    import matplotlib.pyplot as plt
    from mantid.kernel import DateAndTime
    import numpy as np

    def createWorkspaceWithTime(x,y,name,refTimeStamp, delay):
         CreateWorkspace(DataX = x, DataY = y, OutputWorkspace = name, UnitX = 'Wavelength')
         run = mtd[name].getRun()
         start = np.datetime64(refTimeStamp)
         run.setStartAndEndTime(DateAndTime(str(start+int(3600*delay))), DateAndTime(str(start+int(3600*(delay+1)))))
         ConvertToHistogram(InputWorkspace = name, OutputWorkspace = name)

    timeStamp = "2025-07-01T07:00:00"
    createWorkspaceWithTime(np.linspace(1,10,20), np.ones(19), 'ws1',timeStamp, 10)
    #We use default input parameters of the algorithm
    out = HeliumAnalyserEfficiencyTime(InputWorkspace = 'ws1', ReferenceTimeStamp = timeStamp)
    fig, ax = plt.subplots(subplot_kw={'projection': 'mantid'})
    ax.plot(out, wkspIndex=0)
    ax.set_ylabel('Efficiency')
    fig.show()
    # Use plt.show() if running the script outside of Workbench
    #plt.show()


References
----------

.. [#KRYCKA] Polarization-analyzed small-angle neutron scattering. I. Polarized data reduction using Pol-Corr, Kathryn Krycka et al, *Journal of Applied Crystallography*, **45** (2012), 546-553
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_

.. categories::

.. sourcelink::
