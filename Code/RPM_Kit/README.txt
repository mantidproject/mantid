Making a RPM
----------------

1) Copy the latest version of the Mantid source code in to Mantid-X.X/Mantid/src
2) Copy the contents of the Mantid/Images folder in to the Mantid-X.X/Mantid/images directory
3) Copy the latest version of the Mantidplot code in to Mantid-X.X/Mantid/MantidPlot
4) Archive Mantid-X.X using:
	tar -pczf Mantid-X.X.tar.gz Mantid-X.X
5) Copy the tar.gz to /usr/src/redhat/SOURCES/
6) Update the mantid.spec to have the correct version number
7) Run rpmbuild like so:
	rpmbuild -ba mantid.spec

	