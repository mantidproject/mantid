.. _02_lost_the_plot:

=====================
Have I lost the Plot?
=====================

If you've made it this far, then you already know how to Load Data, Execute an Algorithm on it, produce a Plot, open Slice Viewer or Instrument Viewer to Display your data and how to fit an appropriate model. 

Behind the scences, Plot and Workbench are very similar, so you can simply use your knowledge of Workbench if you decide (after discussing with your Instrument Scientist) to use Mantid Plot.

Here's some things worth looking out for:

Where's the script Editor?
--------------------------

While this is front and centre in Workbench, in Plot you have to click the |ScriptEditorbutton.png| button in the top toolbar to open it. 

If want to run a script here, then click Execute > "Execute All" or press "Ctrl+Enter".


Where's the Fit Property Browser?
---------------------------------

- Load a file, such as MUSR00015189.nxs, show it's workspaces by clicking on the triangle beside the *MUSR00015189* workspace and plot any spectrum number of either workspace.
- Click on the |FittingButtonPlot.png| in the top toolbar. Now the Fit Property Browser has opened on the left of the main MantidPlot window. From now on fitting should work pretty much the same as in Workbench. The only difference it that each time you right-click on a plot to "Add a peak", it will ask you which peak type you want.

- Try defining a User function with formula = h*exp(-a*x)
- Set h = 5000 and **Tie** it to this value
- Fit the data

|PlotplotMUSRfit.png|

Other little bits
-----------------

Loading, basic Plotting, and Showing Data work the same as in Workbench. But...

- Editing Plots is done via right-click "Properties", and click on the correct layer (eg. MUSR00015189_1-sp-64). Also there are some options on the top toolbar under Graph, Data and Analysis. Double-clicking on the [1] in the top-left of a plot window will allow you to move curves between plots
- Overall Mantid settings hide within the top toolbar in "View > Preferences"

Using Algorithms and managing Workspaces within their Toolboxes (however rearranged) works the same way, although...
- There is no Plots Toolbox in Mantid Plot. 
- The Messages Box is called the Results Box in Mantid Plot.

.. |FittingButtonPlot.png| image:: /images/FittingButtonPlot.png
.. |ScriptEditorbutton.png| image:: /images/ScriptEditorbutton.png
.. |PlotplotMUSRfit.png| image:: /images/PlotplotMUSRfit.png