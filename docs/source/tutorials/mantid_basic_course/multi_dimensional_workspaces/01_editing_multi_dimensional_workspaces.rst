.. _01_editing_multi_dimensional_workspaces:

=====================================
 Editing Multi Dimensional Workspaces 
=====================================

Where multi-dimensional capabilities are required, Mantid provides a
relatively new set of MDWorkspace (multi-dimensional workspace) types.
MDWorkspaces come in two forms MD Event Workspaces,
which are analogous to :ref:`EventWorkspace` and the
:ref:`MDHistoWorkspace`, which is analogous to the
:ref:`Workspace2D`.

MDEventWorkspaces
=================

These carry the transformed observed events, organised into a tree image
structure. The name of this workspace type is a little misleading, as
the source data does not necessarily have to come from an
EventsWorkspace. |MDWorkspace_structure.png|

MDHistoWorkspaces
=================

:ref:`MDHistoWorkspace` store regularly binned
multi-dimensional data. These are image-only.

Creating MDWorkspaces
=====================

:ref:`algm-ConvertToMD` is an umbrella algorithm that provides a
large number of conversions. The details of all the available options
are outside the scope of this course.

Below is an example of the generation of a 4D MDWorkspace from a direct
run on CNCS.

-  Run **Load** with **Filename** *CNCS_7860_event.nxs*
-  Run **AddSampleLog** with **LogName**\ =\ *Ei*, **LogText**\ =\ *3*,
   **LogType**\ =\ *Number*
-  Run **SetUB** with **a, b and c** = *1.4165*, **u**\ =\ *[1,0,0]*,
   **v**\ =\ *[0,1,0]*
-  Run **SetGoniometer** with **Axis0**\ =\ *Psi,0,1,0,1*
-  Now Run **ConvertToMD** with **QDimensions**\ =\ *Q3D*,
   **QConversionScales**\ =\ *HKL*, **dEAnalysisMode**\ =\ *Direct*,
   **MinValues**\ =\ *[-3,-3,-3,-1]*, **MaxValues**\ =\ *[3,3,3,3]*

or run the following ...

.. raw:: html

   <div style="border:1pt dashed blue; background:#f9f9f9;padding: 1em 0;">

.. code:: python

   ws = Load(Filename='CNCS_7860_event')
   AddSampleLog(Workspace=ws,LogName='Ei',LogText='3.0',LogType='Number')
   SetUB(Workspace=ws,a='1.4165',b='1.4165',c='1.4165',u=[1,0,0],v=[0,1,0])
   AddSampleLog(Workspace=ws,LogName='Psi',LogText='0.0', LogType='Number')
   x = ws.getRun().getLogData('Psi')
   SetGoniometer(Workspace=ws,Axis0='Psi,0,1,0,1')
   mdws = ConvertToMD(InputWorkspace=ws,QDimensions='Q3D',QConversionScales='HKL', dEAnalysisMode='Direct',MinValues=[-3,-3,-3,-1],MaxValues=[3,3,3,3])
   plotSlice(mdws, xydim=["[H,0,0]","[0,K,0]"], slicepoint=[0,0], colorscalelog=True )

.. raw:: html

   </div>

Other sources
-------------

| MDWorkspaces need not be in **Q** at all. Below is an example of image
  data ported into an MDEventWorkspace, where coordinates are in
  real-space.
| |Fly.png|

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Live_Data_Simple_Examples|Mantid_Basic_Course|MBC_MDVisualisation}}

.. |MDWorkspace_structure.png| image:: /images/MDWorkspace_structure.png
.. |Fly.png| image:: /images/Fly.png
   :width: 200px
