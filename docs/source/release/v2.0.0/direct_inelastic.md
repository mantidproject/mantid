(v2.0.0-direct_inelastic)=

# Direct Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## General

- Added ability to load NXSPE files.
- Added support for the HYSPEC instrument.

## Diag improvements

- In this release the diagnostics routines have been updated to match the output of the libisis software.

- As a result of this there is now greater control over the parameters used in the diag process. The help describes these paramters:

```
diagnose(self, white, **kwargs) unbound DirectEnergyConversion.DirectEnergyConversion method
               Run diagnostics on the provided workspaces.

               This method does some additional processing before moving on to the diagnostics:
                 1) Computes the white beam integrals, converting to energy
                 2) Computes the background integral using the instrument defined range
                 3) Computes a total count from the sample

               These inputs are passed to the diagnostics functions

               Required inputs:

                 white  - A workspace, run number or filepath of a white beam run. A workspace is assumed to
                          have simple been loaded and nothing else.

               Optional inputs:
                 sample - A workspace, run number or filepath of a sample run. A workspace is assumed to
                          have simple been loaded and nothing else. (default = None)
                 second_white - If provided an additional set of tests is performed on this. (default = None)
                 hard_mask  - A file specifying those spectra that should be masked without testing (default=None)
                 tiny        - Minimum threshold for acceptance (default = 1e-10)
                 huge        - Maximum threshold for acceptance (default = 1e10)
                 bkgd_range - A list of two numbers indicating the background range (default=instrument defaults)
                 van_out_lo  - Lower bound defining outliers as fraction of median value (default = 0.01)
                 van_out_hi  - Upper bound defining outliers as fraction of median value (default = 100.)
                 van_lo      - Fraction of median to consider counting low for the white beam diag (default = 0.1)
                 van_hi      - Fraction of median to consider counting high for the white beam diag (default = 1.5)
                 van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                             difference with respect to the median value must also exceed this number of error bars (default=0.0)
                 samp_zero    - If true then zeroes in the vanadium data will count as failed (default = True)
                 samp_lo      - Fraction of median to consider counting low for the white beam diag (default = 0)
                 samp_hi      - Fraction of median to consider counting high for the white beam diag (default = 2.0)
                 samp_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                             difference with respect to the median value must also exceed this number of error bars (default=3.3)
                 variation  - The number of medians the ratio of the first/second white beam can deviate from
                              the average by (default=1.1)
                 bleed_test - If true then the CreatePSDBleedMask algorithm is run
                 bleed_maxrate - If the bleed test is on then this is the maximum framerate allowed in a tube
                 bleed_pixels - If the bleed test is on then this is the number of pixels ignored within the
                                bleed test diagnostic
                 print_results - If True then the results are printed to the screen
```

- If print_results=True then the results table will include the number of detectors masked form the hard mask file.

## SofQW2

- The 2D rebinning of SofQW2 has been improved in both speed and accuracy. The gaps on the MARI instrument can be seen clearly and the algorithm appears to do a good job on rebinning on a variety of inelastic instruments (both Direct & Indirect).
- The image below shows a MARI dataset that has been converted to energy transfer and rebinned with Q bins [0,0.03,12] using SofQW2

```{figure} ../../images/2_0_release/SofQW2_MARI.webp
---
class: screenshot
width: 650px
align: right
alt: SofQW2 MARI
---
```

- The output from SofQW2 can fed into ConvertToMDEvents and then display in the SliceViewer where more complex cuts can be made.

```{figure} ../../images/2_0_release/SofQW2_SliceViewer.webp
---
class: screenshot
width: 650px
align: right
alt: SofQW2 SliceViewer
---
```

## Bugs Fixed

- A bug with normalising the short tubes on MERLIN has now been fixed. These tubes should now appear the same shade of colour as the longer tubes.
