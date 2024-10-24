
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the time-independent background over instrument components. It is geared towards TOF spectrometers with position sensitive detector tubes such as IN5 or PANTHER at the ILL and can be readily used as a part of ILL's direct geometry reduction workflow (see :ref:`here <DirectILL>`).

The algorithm works as following:

#. Pick a tube from the given list of tube components
#. For each pixel in the tube, calculate the average Y excluding the elastic peak region
#. Fit a polynomial over the averaged Ys of the pixels using :ref:`CalculatePolynomialBackground <algm-CalculatePolynomialBackground>`
#. Move to the next tube in the list

The *OutputWorkspace* contains the evaluated background polynomials and can be readily used for background subtraction.

The tube components can be given as a list of component names in *Components*, or listed as a comma separated string in the instrument parameters file under the name :literal:`components-for-backgrounds`.

The background exclusion range is determined by the *EPPWorkspace* and *NonBkgRegionInSigmas* properties. *EPPWorkspace* is a table workspace containing the centres of the exclusion regions, usually the elastic peaks of a TOF workspace. Such a workspace can be produced for instance by :ref:`FindEPP <algm-FindEPP>` or :ref:`CreateEPP <algm-CreateEPP>` algorithms. The exclusion region is :literal:`[PeakCentre - NonBkgRegionInSigmas * Sigma, PeakCentre + NonBkgRegionInSigmas * Sigma]` where :literal:`PeakCentre` and :literal:`Sigma` are read from *EPPWorkspace*.

Any pixel for which the :literal:`FitStatus` column  in *EPPWorkspace* is not equal to :literal:`success` is ignored.

A mask workspace produced for example by :ref:`DirectILLDiagnostics <algm-DirectILLDiagnostics>` can be given in the *DiagnosticsWorkspace* property to exclude pixels with known problems. Of course, *InputWorkspace* can be directly masked, too.

Usage
-----
.. include:: ../usagedata-note.txt

**Example - Usage as part of reduction workflow**

.. plot::
   :include-source:

   from mantid.simpleapi import *
   import matplotlib.pyplot as plt
   import numpy as np
   # Load and preprocess some IN5 data
   DirectILLCollectData('ILL/IN5/104007.nxs',
       OutputWorkspace='ws', OutputEPPWorkspace='epp')
   # Get default hard mask + beam stop mask
   DirectILLDiagnostics('ws', OutputWorkspace='diagnostics_mask')
   DirectILLTubeBackground('ws', OutputWorkspace='bkg',
       EPPWorkspace='epp', DiagnosticsWorkspace='diagnostics_mask')
   bkg = mtd['bkg']
   # Apply the background
   ws_bkg_subtracted = Subtract('ws', bkg)
   # Plot the background levels of tubes in the forward direction
   bkg_y = bkg.extractY()
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   xs = np.arange(0, bkg.getNumberHistograms())
   ax.plot(xs, bkg_y[:, 0])
   ax.set_xlim(xmin=70000)
   ax.set_xlabel('Workspace index')
   ax.set_ylabel('Background')
   # Uncomment the following to show the plot
   #fig.show()
   # Remove all workspaces
   mtd.clear()

.. categories::

.. sourcelink::
