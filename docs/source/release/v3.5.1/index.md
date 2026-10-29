(v3.5.1)=

# Mantid 3.5.1 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

| Patch Release     |                |
| ----------------- | -------------- |
| Version           | 3.5.1          |
| Requested by      | Peter Peterson |
| Approved by       | Nick Draper    |
| Authorised on     | 2015-10-11     |
| Code Reviewed by  | Peter Peterson |
| Code Reviewed on  |                |
| Projected release | 2015-11-20     |
| Released on       |                |

## Key Features

This is a patch release to Mantid that corrects some significant issues since the {ref}`last full release <v3.5.0>`.

The main changes are:

- Bugfix for filtering events
- Instrument update url was incorrect.
- `instrument` directory in the install is read only. All user instrument cache files (downloaded IDF and `vtp`) will be in the user's data directory.
- `mantidpython` will revert to running `python` if `--classic` flag is supplied. This allows for returning non-zero signals on error.
- Logging writes console to `stdout` rather than `stdlog`.

### Citing this release

Mantid (2014): Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project. \
<https://doi.org/10.5286/SOFTWARE/MANTID3.5.1>

## Changes in this version

| Issue                                                        | Summary                                                                        | Impact                               | Solution                                    | Side Effect Probability |
| ------------------------------------------------------------ | ------------------------------------------------------------------------------ | ------------------------------------ | ------------------------------------------- | ----------------------- |
| [14030](http://github.com/mantidproject/mantid/issues/14030) | Fix a bug in FilterEvents                                                      |                                      | Clarify variables and use them consistently | medium                  |
| [14058](http://github.com/mantidproject/mantid/issues/14058) | Refl Gui Bug                                                                   |                                      |                                             | low                     |
| [14200](http://github.com/mantidproject/mantid/issues/14200) | Fix the way vertices are inserted in ConvexPolygon                             | Assumption about input               | Remove the assumption                       | low                     |
| [14233](http://github.com/mantidproject/mantid/issues/14233) | Wrong URL for downloading instruments                                          | failed instrument updates            | change URL                                  | low                     |
| [14234](http://github.com/mantidproject/mantid/issues/14234) | Revert to vanilla python in mantidpython                                       |                                      |                                             | low                     |
| [14260](http://github.com/mantidproject/mantid/issues/14260) | Change logging class for console                                               | console logged to unexpected channel | change to stdout                            | low                     |
| [14279](http://github.com/mantidproject/mantid/issues/14279) | Fix another bug in ConvertToReflectometryQ                                     |                                      | Add method to Quadrilateral                 | low                     |
| [14333](http://github.com/mantidproject/mantid/issues/14333) | Fix deprecation warning in simpleapi.py                                        | warning with newer python            | change python rethrow                       | low                     |
| [14340](http://github.com/mantidproject/mantid/issues/14340) | Fix building and packaging on OS X                                             |                                      | change OSX packaging                        | low                     |
| [14358](http://github.com/mantidproject/mantid/issues/14358) | Cleanup OS X packaging                                                         |                                      | change OSX packaging                        | low                     |
| [14355](http://github.com/mantidproject/mantid/issues/14355) | Create USANS_Parameters.xml                                                    |                                      | Add USANS parameters file                   | low                     |
| [14360](http://github.com/mantidproject/mantid/issues/14360) | Fix ISIS autoconfiguration script and separate it from root part of the script |                                      | Modify ISIS only-python scripts             | low                     |
| [14365](http://github.com/mantidproject/mantid/issues/14365) | Instrument directory is read only in the install                               | install area is world writtable      | make it read only                           | medium                  |
| [14405](http://github.com/mantidproject/mantid/issues/14405) | Bug in ipython launch script for Windows                                       |                                      |                                             | low                     |

[on github](https://github.com/mantidproject/mantid/issues?utf8=%E2%9C%93&q=+milestone%3A%22Release+3.5.1%22+)

### Not included

| Issue                                                        | Summary                 | Reason                                                                                                       |
| ------------------------------------------------------------ | ----------------------- | ------------------------------------------------------------------------------------------------------------ |
| [6875](http://github.com/mantidproject/mantid/issues/6875)   | Workspace Documentation | Large amount of changes that don't cleanly merge                                                             |
| [14266](http://github.com/mantidproject/mantid/issues/14266) | LET gets last door      | This requires bringing in some parts of [13566](http://github.com/mantidproject/mantid/issues/13566) as well |

### Are any changes included in the release that are not accounted for above?

No.

### Is the collective effect of the changes acceptably low risk?

Yes.

### Code Reviewer Recommendation

Ship it.
