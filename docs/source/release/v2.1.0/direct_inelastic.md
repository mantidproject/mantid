(v2.1.0-direct_inelastic)=

# Direct Inelastic

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## General

- New algorithm `GenerateGroupingSNSInelastic` to generate grouping/mapping files for SNS direct geometry instruments.

## SofQW3

- New algorithm added that correctly tracks the fractional areas in the overlapping polygon rebinning.
- For areas with no data, they are now treated correctly as NaNs.
