.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithms performs complete treatment of SANS data recorded with the ILL instruments.
This high level algorithm steers the reduction for **multiple** samples measured with one or more detector distances in the most optimal way.
The sample measurements will be corrected for all the instrumental effects and converted to Q-space, producing by default the azimuthal average curve :math:`I(Q)`.
Optionally, it can also perform anisotropic integration with azimuthal wedges or hand-drawn sectors in the instrument viewer.
If requested, it can also produce separate :math:`I(Q)` curves per detector panel.
Note, that panels and azimuthal wedges cannot be requested simultaneously.
The algorithm supports monochromatic (standard, kinetic, and event) as well as TOF modes (linear or variable binning, D33 only).
Makes use of :ref:`SANSILLReduction <algm-SANSILLReduction-v2>` and :ref:`SANSILLIntegration <algm-SANSILLIntegration>` and the algorithm suite used therein.

Output
------

The algorithm will generate a workspace group with the name as provided in the mandatory parameter **OutputWorkspace**.
The group will contain many workspaces as follows:

* The corrected real-space workspaces (2D), one per detector distance
* The integrated I(Q) workspace, one per detector distance
* Optionally, the I(Q) workspaces per azimuthal sector or per detector panel, if requested
* The stitched I(Q) workspace (also for wedges, if requested)
* The calculated scale factors for stitching
* The transmission workspaces, one per wavelength

Notes
-----

* The algorithm will cache the reduced calibration measurements (empty beams, dark currents, empty containers, etc.) in the ADS. This is to ensure that subsequent runs with the same calibrants run faster. However, if a critical parameter is changed, such as the normalisation option, those workspaces must be manually cleared before processing again.
* The sample runs for different distances must be filled in order; that is, if there is only one distance is present, they must be filled in D1, if only 2, then D1 and D2, and so on.
* The transmission runs for different wavelengths must be filled in order; that is, if there is only one wavelength, they must be filled in W1, if 2, then W1 and W2.
* See the section in :ref:`SANSILLReduction <algm-SANSILLReduction-v2>` regarding the blank samples and transmission replicae.

Workflow
--------

Below is the high level flowchart of the algorithm. It calculates transmissions (up to 2 wavelengths), then proceeds to the reduction of sample runs (up to 5 distances).
At each distance, it will load all the samples, concatenate them and then pass through the reduction steps up to azimuthal averaging.
Finally, the I(Q) curves obtained per distance, will be stitched.

.. diagram:: ILLSANS-multiprocess_wkflw.dot

.. categories::

.. sourcelink::
