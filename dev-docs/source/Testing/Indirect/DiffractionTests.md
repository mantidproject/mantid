# Indirect Diffraction Testing

```{contents}
:local:
```

## Diffraction

*Preparation*

- You need access to the ISIS data archive
- `osiris_041_RES10.cal` file from the unit test data

**Time required 2-5 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Diffraction`
2.  Make sure `Instrument` is set to `IRIS`
3.  The default Detector Grouping should be `All`
4.  In `Run Numbers` enter 26176
5.  Click `Run`
6.  The output workspace should have 1 spectra
7.  Change the Detector Grouping to `Groups` and enter 4 groups
8.  Click `Run`
9.  The output workspace should have 4 spectra
10. Change the Detector Grouping to `Custom`, enter `105-108,109-112`
11. Click `Run`
12. The output workspace should have 2 spectra
13. Still using `Custom`, try spectra numbers outside of the range
    105-112. This should not be allowed.
14. Using `Groups`, try more than 8 groups. This should not be allowed.
15. Using `File`, try an empty string. This should not be allowed.

**Time required 2-5 minutes**

------------------------------------------------------------------------

1.  Open `Interfaces` \> `Indirect` \> `Diffraction`
2.  Make sure `Instrument` is set to `OSIRIS`
3.  Set `Reflection` to `Diffonly`
4.  In `Run Numbers` enter 89813
5.  In `Vanadium File` enter 89757
6.  In `Cal File` load the `osiris_041_RES10.cal` file from the unit
    test data
7.  Select Detector Grouping to be `All`
8.  Click `Run`
9.  There should be three output workspaces, ending in `_dRange`, `_q`
    and `_tof`
10. Each of the output workspaces should have 1 spectra
11. Change the Detector Grouping to `Groups` and enter 5 groups
12. Click `Run`
13. Each of the output workspaces should have 5 spectra
14. Change the Detector Grouping to `Custom`, enter `3-500,501-962`
15. Click `Run`
16. Each of the output workspaces should have 2 spectra
