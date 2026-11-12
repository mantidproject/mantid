(v3.4.1)=

# Mantid 3.4.1 Release Notes

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

| Patch Release    |                |
| ---------------- | -------------- |
| Version          | 3.4.1          |
| Requested by     | Peter Peterson |
| Approved by      | Nick Draper    |
| Authorised on    | 21-Jul-2015    |
| Code Reviewed by | Peter Peterson |
| Code Reviewed on | 18-22 Aug 2015 |
| Released on      | 28 Aug 2015    |

## Key Features

This is a patch release to Mantid that corrects some significant issues since the {ref}`last full release <v3.4.0>`.

The main changes are:

- The URIs and packet format changed for ADARA instruments.
- Fixes to the `mantidpython` script on RHEL6.
- Updated instrument geometries for BASIS, CNCS, MANDI, and POWGEN.

### Citing this release

Mantid (2014): Manipulation and Analysis Toolkit for Instrument Data.; Mantid Project. \
<https://doi.org/10.5286/SOFTWARE/MANTID3.4.1>

## Changes in this version

| Ticket                                                       | Summary                                                          | Impact | Solution | Probability of side effects |
| ------------------------------------------------------------ | ---------------------------------------------------------------- | ------ | -------- | --------------------------- |
| [789](http://github.com/mantidproject/mantid/issues/789)     | Minor bug in ISIS direct inelastic reduction                     |        |          |                             |
| [12757](http://github.com/mantidproject/mantid/issues/12757) | Assign dimensions to the dectectors in the BASIS instrument file |        |          |                             |
| [12861](http://github.com/mantidproject/mantid/issues/12861) | Sum monitors signals to improve GetEI accuracy                   |        |          |                             |
| [13074](http://github.com/mantidproject/mantid/issues/13074) | Edit Fixing bug in wrapper script on rhel6                       |        |          |                             |
| [13167](http://github.com/mantidproject/mantid/pull/13167)   | Fix jenkins mpi build                                            |        |          |                             |
| [13175](http://github.com/mantidproject/mantid/pull/13175)   | Adding symbolic links to startup scripts to /usr/bin             |        |          |                             |
| [13246](http://github.com/mantidproject/mantid/pull/13246)   | Update ADARA files with latest version                           |        |          |                             |
| [13389](http://github.com/mantidproject/mantid/pull/13389)   | New powgen geometry for the 2015 cycle                           |        |          |                             |
| [13395](http://github.com/mantidproject/mantid/pull/13395)   | Changing URIs for ADARA instruments                              |        |          |                             |
| [13414](http://github.com/mantidproject/mantid/pull/13414)   | New CNCS geometry                                                |        |          |                             |
| [13429](http://github.com/mantidproject/mantid/pull/13429)   | New Mandi geometry                                               |        |          |                             |

Not added are: [846](http://github.com/mantidproject/mantid/issues/846), [12915](http://github.com/mantidproject/mantid/pull/12915), and [13075](http://github.com/mantidproject/mantid/issues/13075)

### Are any changes included in the release that are not accounted for above?

### Is the collective effect of the changes acceptably low risk?

### Code Reviewer Recommendation
