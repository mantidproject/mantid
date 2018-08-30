.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The purpose of this algorithm is to provide users a tool to control the
workflow to refine powder diffractometer's peak profile. For
Time-Of-Flight powder diffractometers in SNS, back-to-back exponential
convoluted with pseudo-voigt profile functions are used. The function is
complicated with strong correlated parameters. Thus, the refinement on
these parameters contains multiple steps, within each of which only a
subset of profile parameters are refined.

In order to control the workflow, there are four major functions
supported by this algorithm

- **Setup**  : set up a few workspaces that will be used to refine profile parameters in multiple steps;

- **Refine**  : select a subset of peak parameters and do Le Bail fit on them;

- **Save**  : save the current refinement status and refinement history to a project file;

- **Load**  : set up a few workspaces used for refining by loading them from a previously created project file.



Input and output workspaces
###########################



- InputWorkspace : data workspace containing the diffraction pattern to refine profile parameters with;

- SeqControlInfoWorkspace : table workspace used to track refinement. Below is the introduction on the fields\/columns of this workspace.

- *Step* : refinement step. User can start a refinement from the result of any previous step;
  
- *OutProfile* : name of the table workspace containing refined profile parameters;
  
- *OutReflection* : name of the table workspace containing Bragg peaks' peak parameters calculated from refined parameters value;
  
- *OutBackgroud* : name of the table workspace containing the value of the output background parameter;
  
- *OutBckgroundParam* : name of the output background parameters;
  
- *Refine* : profile parameters that are refined in this step;

- *RwpOut* : output Rwp from refinement;

- *LastStep* : last step where this step is based on;

- *RwpIn* : input Rwp

- *InProfile* : input profile parameter workspace's name;

- *InReflection* : input Bragg peak parameters workspace' name;

- *InBackgroud* : input background workspace;

- *InBckgroundParam* : input background parameters.


- InputProfileWorkspace : table workspace contraining starting values of profile parameters;

- InputBraggPeaksWorkspace : table workspace containing the Bragg peaks' information for Le Bail fit;

- InputBackgroundParameterWorkspace : table workspace containing the background parameters' value



Supported peak profiles
#######################

- Neutron Back-to-back exponential convoluted with pseudo-voigt : Fullprof profile 9 and GSAS TOF profile 3;

- Thermal neutron Back-to-back exponential convoluted with pseudo-voigt: Fullprof profile 10 (a.k.a. Jason Hodges function).


Supported background types
##########################

- Polynomial

- Chebyshev

- FullprofPolynomial

Hint for using
--------------

This is just a brief description for how to use this algorithm.

1. *Setup*;
2. *Refine*: refine *Dtt1* and *Zero* from step 0;
3. *Refine* : reifne *Alph0*  and  *Beta0*  from step 1;
4. *Refine* : refine *Alph1*  from step 1 with failure;
5. *Refine* : refine *Beta1*  from step 1 because step 2 fails;
6. *Refine* : refine *Sig-1*  from last step;
7. *Save* : save current work and history to a Nexus file.

.. categories::

.. sourcelink::
