This folder contains the .Properties files that are to be used for release with the software.

The installer corrects certain file paths to match those of the final configuration:
these are
	plugins.directory
	pythonscripts.directory
	instrumentDefinition.directory

Properties files for use in automated and development testing are in the build\tests directory.

Note:  While there is a user.properties file defined here this will only be userd for the first 
installation to a client, subsequent installs will not overwrite the clients copy.