(v2.1.0-python)=

# Python

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org). Trac ticket numbers are given for reference alongside the GitHub issue the ticket was migrated to.
```

## Known Issues

See known Python issues.

## Multiple Script Execution

[#5852](https://github.com/mantidproject/mantid/issues/5852) (Trac 5006)

In all previous versions of Mantid, only a single Python script could be executed at at time. In addition, any long running pure python code would cause the GUI to hang.

This version of Mantid comes with the ability to execute many Python scripts simultaneously (this also includes the script interpreter). As a by product of these changes any single script running long bits of pure python code will no longer hang the GUI, as they use to. Most of the changes to make this happen have occurred under the hood and should go largely unnoticed.

Notable changes to be aware of:

- each script now has its own output window rather each script sharing a single window;
- a script cannot be edited while it is being executed. You can of course still edit other scripts;

```{figure} ../../images/2_1_release/MultipleScriptsZoomed.webp
---
class: screenshot
width: 1000px
align: right
alt: MultipleScriptsZoomed
---
```

## Helping migration from v1 to v2 (first cut)

A script has been introduced to aid in migration from the first to the second versions of the Python API. It is a first cut and handles only:

- the simpler cases of direct string replacements, e.g. `from mantidsimple import *` {math}`\rightarrow` `from mantid.simpleapi import *`
- algorithm calls that do not capture a return value, e.g. `Load('runfile.ext','runWS')` {math}`\rightarrow` `testWS = Load(Filename='runfile.ext')`

More details can be found in the [changes](http://download.mantidproject.org/docs/current-release/python/html/changes.html) document.

## Python algorithms exposed to new API

In the previous release the new Python API was introduced but it was not possible to create Python algorithms with it. This functionality has now been implemented and brings to new API one step closer to being able to replace the old API in all situations. This will mean at some point it can be remvoed.

A tutorial for creating a Python algorithm with the new API can be found here: Python Algorithms Documentation

## Notable Bug Fixes

### Segfault if starting a python algorithm while a script is running

[#5760](https://github.com/mantidproject/mantid/issues/5760) (Trac 4914)

As a consequence of the multiple-script execution this bug was eradicated as multiple snippets of python code can be executed at the same time quite happily.

#### Python intellisense window disappears with mouse click (on windows)

[#5264](https://github.com/mantidproject/mantid/issues/5264) (Trac 4417)

In the previous version of Mantid there was a bug in the script editor component of mantid that meant the auto-completion box did not respond correctly to mouse clicks. The underlying library has been updated on Windows and this functionality now works as expected.
