Making a RPM
----------------

1) Copy the latest version of the Mantid source code in to Mantid-X.X/Mantid/src
2) Archive Mantid-X.X using:
	tar -pczf Mantid-X.X.tar.gz Mantid-X.X
3) Copy the tar.gz to /usr/src/redhat/SOURCES/
4) Update the mantid.spec to have the correct version number
5) Run rpmbuild like so:
	rpmbuild -ba mantid.spec

	