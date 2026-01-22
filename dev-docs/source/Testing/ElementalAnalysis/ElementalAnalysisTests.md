# Elemental Analysis GUI Testing

```{contents}
:local:
```

## Introduction

This document outlines some basic tests for the new elemental analysis
GUI. Please note that it is currently hidden from users.

## Set up

To do the testing you will need some data (please request this from the
ISIS spectroscopy team). You will need to add the file location to the
search paths for Mantid.

### Basic tests

When the GUI starts up it will have some dummy widgets, these do not do
anything. The bottom of the GUI will have a
<span class="title-ref">Manage user directories</span> and help buttons.
The tabs should be dockable, closing them will result in the tab
returning to the docked state. Closing the GUI should also close any
undocked tabs.
