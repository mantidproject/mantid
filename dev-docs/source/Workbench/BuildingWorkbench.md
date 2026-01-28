# Workbench Development

## Building

The following build instructions assume you have followed the
instructions on the [Getting Started](GettingStarted) pages and can build mantid. To
enable the build of the workbench simply set the cmake flag
`-DENABLE_WORKBENCH=ON` (current default) and build as normal. A
`workbench` startup script (Linux/macOS) or executable (Windows) will
appear in the `bin` folder. For Windows the executable will appear in
the configuration subdirectory of `bin`.

## Packaging

Packaging is currently supported on all platforms that Mantid works with
(Windows, RHEL7, Ubuntu 20.04, macOS).
