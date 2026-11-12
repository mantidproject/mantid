(v2.0.0-installer)=

# Installation

```{note}
These release notes were migrated from the retired Mantid wiki (archive.mantidproject.org).
```

## Windows Installer

The windows installer now picks the drive that contains windows as the default installation drive. This will not have affected many people in the past as they only have one drive, but those with a large USB drive attached will appreciate this change.

The windows installer has been changed so that it no longer requires you to be an administrator on your computer to install Mantid. However you will not see this benefit for this release as the first action of the installer is to uninstall the previous version before installing the new one. The previous versions uninstallation still requires you to be an administrator (just this last time). Releases after this should not require administrator privileges for installation.

Note: If you see "error code 2349" during installation, it is because you are not running it as an administrator.

### VATES Installer

Please note the seperate (optional) VATES installer does not yet uninstall previous versions, so you still need to uninstall the last version first using the control panel.

## Multiple RHEL6 versions at SNS and HFIR

To aid in getting out new features and bug fixes to users there are now three versions of mantid installed at SNS:

- **MantidPlot** - The latest stable release.
- **MantidPlotNightly** - The results of the last nightly build with all of unit tests and system tests that passed.
- **MantidPlotUnstable** - The results of the last clean build that successfully ran all of the unit tests. This is nominally every two hours.

All three of these versions can be started with the name in all lower case (e.g. "mantidplotnightly").

Note: If you want to use python or paraview outside of mantid with a version other than the stable release you need to run the appropriate shell script. This can be done in your bashrc by adding

```
if [ -f /opt/mantidnighly/etc/mantid.sh ]; then
  . /opt/mantidnightly/etc/mantid.sh
fi
```
