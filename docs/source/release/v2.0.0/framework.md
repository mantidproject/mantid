(v2.0.0-framework)=

# Framework

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Group Workspaces

Group workspaces now work correctly with all of the binary algorithms (Plus, Minus, Multiply & Divide), both from the user interface and when working with Python.

Group workspaces are now indexable in both the old and new python APIs. Indexed group workspaces will return the contained workspace at that index. To support this feature, in both python APIs group workspaces also return a size in terms of the number of contained workspaces so that users can run loop operations over their workspace groups.

## Projects

Mantid projects are now saved to a folder that contains all of the files involved.

## ISIS Event Nexus Files

Some peculiarities of the data storage of events in ISIS nexus files have been adjusted for in this version of Mantid, see {ref}`isis event files - randomisation <v2.0.0-isis_event_files_randomization>` for details. Newer Nexus files have this change in the file itself, Mantid detects this and does no perform the randomisation in this case.

## Signed Theta

It's now possible to work with signed theta values. Instruments supporting a ReferenceFrame in their xml definiton and applying the signed_theta Instrument Parameter may work on the basis of signed theta. This had been an issue for our reflectometry users and is fixed in the latest release.

## Stability

We have tracked down and removed a variety of memory leaks and race conditions that were possible in the code to give much better long term stability of the framework.

```{toctree}
---
hidden:
---
isis_event_files_randomization
```
