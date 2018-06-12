.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses :math:`E=\frac{1}{2}mv^2`
to calculate the energy of neutrons leaving the
source. The velocity is calculated from the time it takes for the
neutron pulse to travel between the two monitors whose spectra were
specified. If no spectra are specified, the algorithm will use the
defaults for the instrument.

An initial energy guess is required for the algorithm to find the
correct peak. The analysis will be done on the highest peak that is
within 8% of the estimated TOF given by the estimate. If no initial
guess is given, the algorithm will try to get it from the workspace,
from a sample log variable called *EnergyRequest*.

Not all neutrons arrive at the monitors at the same time because their
kinetic energies, and therefore velocities, are all different. The time
of arrival of the neutron pulse is taken to be the mean of the two half
peak height locations. The half height points are found as follows:

#. the peak height is the largest number of counts above the background
   in any bin in the window
#. the half height is half the above number
#. examine bins to the left of the bin with the highest number of counts
   looking for a bin with less than half that number above background
#. interpolate between this point bin and the one immediately previous
   to find the first half height location
#. repeat the steps 3 and 4 looking to the right of the highest point to
   get the second half height point
#. the mean of the X-values of the two half height points is the TOF
   arrival time of the neutrons

The above process is illustrated on a peak is shown below in the image
below.

.. image:: /images/Monitorspect_getei.jpg
    :align: center
    :width: 618px
    :alt: Monitor Peak

The distances between the monitors are read from the instrument
definition file. It is assumed that the source and the monitors all lie
on one line and that the monitors have the same delay time.

Usage
-----

.. include:: ../usagedata-note.txt

**Example: Fixing the Ei**

.. testcode:: fixEi
    
    ws = CreateSampleWorkspace(bankPixelWidth=1,binWidth=10)

    (ei, firstMonitorPeak, FirstMonitorIndex, tzero) = GetEi(ws,Monitor1Spec=1,Monitor2Spec=2,EnergyEstimate=15.0,FixEi=True)

    print("ei: {:.2f}".format(ei))
    print("firstMonitorPeak: {:.2f}".format(firstMonitorPeak))
    print("FirstMonitorIndex: {}".format(FirstMonitorIndex))
    print("tzero: {:.2f}".format(tzero))

Output:

.. testoutput:: fixEi
    :options: +NORMALIZE_WHITESPACE

    ei: 15.00
    firstMonitorPeak: 8854.69
    FirstMonitorIndex: 0
    tzero: 0.00

**ISIS Example**

.. testcode:: ExIsis

    ws = Load("MAR11001.raw")
    # Workspace contains monitors
    vals = GetEi(ws, EnergyEstimate=12)
    # Output from algorithm is a tuple of the following values:
    # (IncidentEnergy, FirstMonitorPeak, FirstMonitorIndex, TZero)
    print("Calculated Incident Energy = {:.10f} meV".format(vals[0]))
    print("First Monitor Peak = {:.8f} microseconds".format(vals[1]))

Output

.. testoutput:: ExIsis

    Calculated Incident Energy = 12.9728953307 meV
    First Monitor Peak = 6536.70777852 microseconds

**SNS Example**

CNCS and HYSPEC do not actually calculate the incident energy, but use the
*EnergyRequest* log value as the calculated incident energy. ARCS and SEQUOIA,
however, do perform the calculation for the incident energy. Also, SNS instruments
use the negative of the TZero output value in further calculations.

.. testcode:: ExSns

    ws = Load("CNCS_7860_event.nxs", LoadMonitors=True)
    # Need monitor workspace, as main workspace does not.
    # Energy estimate not manditory for SNS instruments
    vals = GetEi(ws[1])
    print("Calculated Incident Energy = {:.1f} meV".format(vals[0]))
    print("Time Zero = {:.10f} microseconds".format(-vals[3]))

Output:

.. testoutput:: ExSns

    Calculated Incident Energy = 3.0 meV
    Time Zero = -61.7708018029 microseconds

.. categories::

.. sourcelink::
