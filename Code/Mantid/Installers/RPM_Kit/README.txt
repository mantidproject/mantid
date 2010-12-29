Making a RPM
------------

Unless you plan to build the rpm files as root you will need to do the
following:

(1) Create a   ~/.rpmmacros   file with a line similar to the following

%_topdir	/home/freddie/mybuilds

(note: that is a tab separating the two parts)

(2) create the corresponding RPM build directories

    cd /home/freddie/mybuilds
    mkdir BUILD RPMS SOURCES SPECS SRPMS
    cd RPMS
    mkdir i386 i486 i586 i686 x86_64

(i.e. this should look like the default build area /usr/src/redhat)

(3) Now you should be able to run the  make_rpm  script - this
    will generate a Mantid-*.tar.gz via a subversion export 
    and then build src and binary rpms in the "mybuilds" area

    type      

           sh make_rpm.sh -h

    for more information
