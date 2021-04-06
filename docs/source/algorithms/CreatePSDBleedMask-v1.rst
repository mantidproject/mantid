.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm identifies tubes, which become saturated due to high
neutron flux from intense Bragg peaks affecting these tubes,
so these tubes are not counting neutrons but show constant high
counts over the tube in the elastic line(s) energy region(s).
After that, the algorithm generates masking workspace to
eliminate such tubes from the final results.

The bleeding effect occurs due to some old data acquisition electronics and
observed as homogeneous high counts reading, symmetric over the whole
tube length. The bleeding occurs at or close to incident energy or
specific energies in multirep mode, where bright Bragg reflections
hit the tube in positions, symmetric with respect to the tube centre.

The example of the workspace, affected by the bleed current and requesting
bleeding corrections is presented on the following picture:

.. image:: /images/BleedingSignal.png


First of all, the algorithm attempts to find all tubes
within the instrument attached to the workspace. If successful,
each tube is tested for saturation above the level defined by the
production of *MaxTubeFramerate* property by the *goodfrm* log value,
retrieved from the workspace.

If the total signal, summed over all tubes pixels, excluding the number
of central pixels specified by the *NIgnoredCentralPixels* property,
exceeds the threshold, specified by the *MaxTubeFramerate* multiplied
by the *goodfrm* value, the tube becomes masked.

The following image shows two instrument views obtained for two
converted to energy transfer workspaces, recorded for two incident
energies in multirep mode. Second energy range suffers from
bleeding signal as on the picture above, so proper
bleeding corrections are calculated. First energy range does not
show bleeding signal, so no bleeding corrections applied to it.

.. image:: /images/BleedingCorrections.png

Restrictions and requirements to the input workspace
####################################################

-  The workspace must contain *goodfrm* log with value specifying the number of good frames recorded by the instrument.

-  The workspace should not be normalized as *goodfrm* is proportional to neutron flux over workspace so the *MaxTubeFramerate* have the meaning of the frame rate only for non-normalized workspaces. To obtain consistent results in a case when a workspace is normalized, user should divide *MaxTubeFramerate* by the normalization factor.


Usage
-----

**Example:**

.. testcode:: ExPSDMask

    import numpy as np

    ws=CreateSampleWorkspace()
    AddSampleLog(ws,"goodfrm","10","Number")
    noisyY =  np.array(ws.readY(0))
    noisyY[0]=1e20
    ws.setY(50,noisyY)
    (wsOut, numFailures) = CreatePSDBleedMask(ws,MaxTubeFramerate=10, NIgnoredCentralPixels=2)

    print("{} spectra have been masked in wsOut".format(numFailures))


Output:

.. testoutput:: ExPSDMask

    10 spectra have been masked in wsOut



.. categories::

.. sourcelink::
