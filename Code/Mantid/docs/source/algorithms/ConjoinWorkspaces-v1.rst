.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm can be useful when working with large datasets. It
enables the raw file to be loaded in two parts (not necessarily of equal
size), the data processed in turn and the results joined back together
into a single dataset. This can help avoid memory problems either
because intermediate workspaces will be smaller and/or because the data
will be much reduced after processing.

The output of the algorithm, in which the data from the second input
workspace will be appended to the first, will be stored under the name
of the first input workspace. Workspace data members other than the data
(e.g. instrument etc.) will be copied from the first input workspace
(but if they're not identical anyway, then you probably shouldn't be
using this algorithm!). Both input workspaces will be deleted.

Conflict Spectrum IDs
#####################

The algorithm adds the spectra from the first workspace and then the
second workspace.

-  The original spectrum IDs will be respected if there is no conflict
   of spectrum IDs between the first workspace and thfiree second.
-  If there are conflict in spectrum IDs, such that some spectrum IDs
   appear in both workspace1 and workspace2, then it will be resolved
   such that the spectrum IDs of spectra coming from workspace2 will be
   reset to some integer numbers larger than the largest spectrum ID of
   the spectra from workspace1. Assuming that the largest spectrum ID of
   workspace1 is S, then for any spectrum of workspace wi in workspace2,
   its spectrum ID is equal to (S+1)+wi+offset, where offset is a
   non-negative integer.


Restrictions on the input workspace
###################################

The input workspaces must come from the same instrument, have common
units and bins and no detectors that contribute to spectra should
overlap.

Exception
#########

If property 'CheckOverlapping' is set to true, and there are spectra and/or detectors
are overlapping between two input workspaces,
then an 'invalid_argument' exception will be thrown from function 'CheckForOverlap'.

Usage
-----

**ConjoinWorkspaces Example**

.. testcode:: ConjoinWorkspacesEx

    ws1 = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=2, BankPixelWidth=1, BinWidth=10, Xmax=50)
    print "Number of spectra in first workspace", ws1.getNumberHistograms()
    ws2 = CreateSampleWorkspace(WorkspaceType="Histogram", NumBanks=3, BankPixelWidth=1, BinWidth=10, Xmax=50)
    print "Number of spectra in second workspace", ws2.getNumberHistograms()
    ConjoinWorkspaces(InputWorkspace1=ws1, InputWorkspace2=ws2, CheckOverlapping=False)
    print "Number of spectra after ConjoinWorkspaces", mtd['ws1'].getNumberHistograms()

Output:

.. testoutput:: ConjoinWorkspacesEx

    Number of spectra in first workspace 2
    Number of spectra in second workspace 3
    Number of spectra after ConjoinWorkspaces 5

.. categories::
