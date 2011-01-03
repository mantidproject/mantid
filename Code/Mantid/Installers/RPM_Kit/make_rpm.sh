#!/bin/sh
# 
# @file make_rpm.sh RPM build script for Mantid
# @author Freddie Akeroyd, STFC ISIS Facility
# @date 30/03/2009
#
usage() { 
cat << EOF
Build an rpm file for Mantid

Usage: sh make_rpm.sh [-l] [-r rev]

    -l       Use local Mantid build tree (default: chekout new svn version)
    -r rev   Use rev rather than HEAD as the subversion revision
EOF
}

do_local=false
svn_rev="HEAD"
while getopts "hlr:" Option
do
    case $Option in
        l)
	    do_local=true
	    ;;
        r)
	    svn_rev=$OPTARG
	    ;;
        h)
	    usage
	    exit
	    ;;
	*)
	    echo "Invalid argument $Option"
	    ;;
    esac
done
shift $(($OPTIND - 1))

# Get Mantid version from MantidVersion.txt file
cd ../..
mantid_versionfile=`pwd`/MantidVersion.txt
cd Installers/RPM_Kit
if test -f $mantid_versionfile; then
    mantid_version=`cat $mantid_versionfile`
else
    echo "Error: Cannot open $mantid_versionfile"
    exit 1
fi
#
mantid_svn="http://svn.mantidproject.org/mantid/trunk"
rm -f Mantid-$mantid_version.tar.gz
rm -fr Mantid-$mantid_version
if $do_local; then
    svn_version=`svnversion --no-newline`
    echo "Linking to local Mantid version ..."
    cp -r Mantid Mantid-$mantid_version
    cd Mantid-$mantid_version
    # we use `pwd` to make absolute links; that way the link will
    # still work when the area is tarred up
  #RJT, 03/01/2011 - These paths need fixing but I'm not sure what the starting point is for these relative paths
    ln -s `pwd`/../../../Images .
    ln -s `pwd`/../../../Test/Instrument instrument
    cd Code
    ln -s `pwd`/../../../qtiplot .
    ln -s `pwd`/../../../Mantid .
    cd ../..
else
    svn_version="$svn_rev"
    echo "Exporting mantid source revision $svn_rev from subversion ..."
    echo "Directory structure ..."
    while ! svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Code/RPM_Kit/Mantid Mantid-$mantid_version; do echo "Retrying svn"; rm -fr Mantid-$mantid_version; sleep 30; done
#
    cd Mantid-$mantid_version
    echo "Images ..."
    while ! svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Code/Mantid/Images Images; do echo "Retrying svn"; rm -fr Images; sleep 30; done
    echo "MantidPlot ..."
    while ! svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Code/qtiplot Code/qtiplot; do echo "Retrying svn"; rm -fr Code/qtiplot; sleep 30; done
    echo "src ..."
    while ! svn -q --non-interactive export --force -r "$svn_rev" $mantid_svn/Code/Mantid Code/Mantid; do echo "Retrying svn"; rm -fr Code/Mantid; sleep 30; done
    echo "instrument ..."
    while ! svn -q --non-interactive export --force -r "$svn_rev" $mantid_svn/Code/Mantid/Instrument instrument; do echo "Retrying svn"; rm -fr instrument; sleep 30; done
    cd ..
fi
#mantid_release="0.`date +%Y%m%d`svnR$svn_version"
mantid_release="0.svnR${svn_version}.`date +%Y%m%d`"
echo "Exporting complete - making spec file for version $mantid_version release $mantid_release"
#
# Set correct path to python
py_sitepackages=`python python_sitepackages.py`
sed -e "s/MANTID_VERSION/$mantid_version/" -e "s/MANTID_RELEASE/$mantid_release/" -e "s/SVN_VERSION/$svn_version/" -e "s@PYTHON_SITEPACKAGES@$py_sitepackages@" < mantid.spec.template > Mantid.spec
echo "Removing old tar files"
rm -f *.tar.gz
echo "Building tar file Mantid-$mantid_version.tar.gz"
tar -cpzf Mantid-$mantid_version.tar.gz Mantid-$mantid_version
topdir=`rpm --showrc|grep topdir| awk '{print $3}' | tail -1`
if test ! -e "$topdir"; then
    echo "Unable to determine RPM topdir from rpmrc; assuming /usr/src/redhat"
    topdir="/usr/src/redhat"
fi
if test ! -w "$topdir"; then
    echo "ERROR: RPM build directory not writable - check README.rpm"
    exit
fi
mytop=`pwd`
ln -sf $mytop/Mantid-$mantid_version.tar.gz $topdir/SOURCES
cp -f Mantid.spec $topdir/SPECS
rm -f Mantid.spec
cd $topdir/SPECS
echo "Building RPM file in $topdir/RPMS"
if test -e /etc/debian_version; then
    rpmbuild --nodeps -ba Mantid.spec
else
    rpmbuild -ba Mantid.spec
fi
echo "Building complete - check $topdir/RPMS"
