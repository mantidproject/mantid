Inelastic Corrections Testing
=============================

.. contents::
   :local:

*Prerequisites*

- Download the `ISIS Sample Data <http://download.mantidproject.org>`_

Container Subtraction tab
-------------------------

**Time required 1 - 3 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Corrections``
#. Go to the ``Container Subtraction`` tab
#. With the Sample combo box set to ``File`` click browse and select the file ``irs26176_graphite002_red`` from the Sample Data folder
#. With the Container combo box set to ``File`` click browse and select the file ``irs26174_graphite002_red`` from the Sample Data folder
#. Click ``Run``
#. This should plot a blue subtracted line on the embedded plot. A workspace ending in ``_red`` should be produced
#. Change the ``Spectrum`` value and the plot should be updated with the corresponding workspace index
#. Check that when clicking ``Plot Current Preview``, it will open a pop-up window with the current preview plot
#. In the ``Output Options``, select ``Open Slice Viewer`` using the down arrow, it should open the Slice Viewer with the generated dataset
#. In the ``Output Options``, select ``Plot 3D Surface`` using the down arrow, it should open a surface plot for the generated dataset
#. In the ``Output Options``, select indices ``50-51``. A red asterisk should appear, preventing you from using ``Plot Spectra``
#. In the ``Output Options``, select indices ``49-50``. It should be possible to click ``Plot Spectra`` to plot a two spectra graph

Calculate Monte Carlo Absorption Tab
------------------------------------

**Time required 3 - 5 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Corrections``
#. Go to the ``Calculate Monte Carlo Absorption`` tab
#. With the Sample combo box set to ``File`` click browse and select the file ``irs26176_graphite002_red`` from the Sample Data folder
#. Choose the Sample Shape to be ``Flat Plate``
#. Sample Height & Width should be ``1.0``
#. Sample and Container Thickness should be ``0.1``
#. Sample mass density should be ``1.0``. The Sample formula should be ``H2-O``
#. Container mass density should be ``6.0``. The Container formula should be ``V``
#. Click ``Run`` and wait.
#. A group workspace ending in ``_Corrections`` should be created. Keep this workspace for the Apply Absorption Corrections test.
#. In the ``Output Options``, the combo box should have four entries ending in ``_ass``, ``_assc``, ``_acsc`` and ``_acc``.
#. In the ``Output Options``, try using the ``Plot Wavelength`` and ``Plot Angle`` options.

Apply Absorption Corrections Tab
--------------------------------

**Time required 1 - 3 minutes**

--------------

#. Go to ``Interfaces`` > ``Inelastic`` > ``Corrections``.
#. Go to the ``Apply Absorption Corrections`` tab.
#. With the Sample combo box set to ``File`` click browse and select the file ``irs26176_graphite002_red`` from the Sample Data folder.
#. With the Corrections combo box set to ``Workspace``, and select the workspace ending in ``_Corrections`` from the previous test.
#. Tick ``Use Container`` and select the file ``irs26174_graphite002_red`` from the Sample Data folder.
#. Click ``Run`` and wait.
#. This should plot a blue subtracted line on the embedded plot. A workspace ending in ``_red`` should be produced
#. In the ``Output Options``, select ``Open Slice Viewer`` using the down arrow, it should open the Slice Viewer with the generated dataset
#. In the ``Output Options``, select ``Plot 3D Surface`` using the down arrow, it should open a surface plot for the generated dataset
#. In the ``Output Options``, select indices ``50-51``. A red asterisk should appear, preventing you from using ``Plot Spectra``
#. In the ``Output Options``, select indices ``49-50``. It should be possible to click ``Plot Spectra`` to plot a two spectra graph
