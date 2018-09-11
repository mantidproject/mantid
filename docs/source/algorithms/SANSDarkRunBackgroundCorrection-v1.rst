.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm subtracts a dark run from a workspace. *InputWorkspace* and *DarkRun* have to
be of type Workspace2D and need to contain the same spectra.
The user can choose to either subtract spectra which are assoicated with detecors 
(*ApplyToDetectors*) and/or monitors (*ApplyToMonitors*). In the case of monitors, the user can 
select specific monitors (*SelectedMonitors*) according to their detecotor IDs.

The *NormalizationRatio* is used to scale the signal values of the *DarkRun* workspace before
subtraction.

The background subtraction can be performed in several ways.

* *Uniform* disabled: *DarkRun* is subtracted bin by bin from the *InputWorkspace*.
* *Uniform* enabled: An average value for each spectra of the *DarkRun* is calculated. This average value is subtracted from the corresponding spectrum of the *InputWorkspace*. Note that *Mean* cannot be enabled when *Uniform* is disabled.
* *Mean* enabled: This calculates an average over all spectra. This average is subtracted from all the spectra
* *Mean* disabled: The subtraction happens for all spectra separately.

Usage
-----

**Example - SANSDarkRunBackgroundCorrection for**

.. testcode:: SANSDarkRunBackgroundCorrection

    # Create sample workspaces. Note that the dark run is here the same as the sample run
    ws_sample = CreateSampleWorkspace()
    ws_dark_run = CloneWorkspace(ws_sample)

    out_ws = SANSDarkRunBackgroundCorrection(InputWorkspace = ws_sample, 
                                             DarkRun = ws_dark_run,
                                             NormalizationRatio = 0.5,
                                             Uniform = False,
                                             Mean = False,
                                             ApplyToDetectors = True,
                                             ApplyToMonitors = False)

    # We should have effectively halfed the data values
    in_y = ws_sample.dataY(0)
    out_y = out_ws.dataY(0)

    print("The first bin of the first spectrum of the input was {:.1f}".format(in_y[0]))
    print("After the dark run correction it is {:.2f}".format(out_y[0]))

Output:

.. testoutput:: SANSDarkRunBackgroundCorrection

    The first bin of the first spectrum of the input was 0.3
    After the dark run correction it is 0.15


.. categories::

.. sourcelink::
