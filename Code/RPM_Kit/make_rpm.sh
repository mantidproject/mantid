#!/bin/sh
# 
# @file make_rpm.sh RPM build script for Mantid
# @author Freddie Akeroyd, STFC ISIS Facility
# @date 30/03/2009
#
function usage { 
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
#
mantid_version="1.0"
svn_version=`svnversion --no-newline`
#mantid_release="0.`date +%Y%m%d`svnR$svn_version"
mantid_release="0.svnR${svn_version}.`date +%Y%m%d`"
#
mantid_svn="http://svn.mantidproject.org/mantid/trunk"
rm -f Mantid-$mantid_version.tar.gz
rm -fr Mantid-$mantid_version
if $do_local; then
    echo "Linking to local Mantid version ..."
    cp -r Mantid Mantid-$mantid_version
    cd Mantid-$mantid_version
    # we use `pwd` to make absolute links; that way the link will
    # still work when the area is tarred up
    ln -s `pwd`/../../../Images .
    ln -s `pwd`/../../../Test/Instrument instrument
    cd Code
    ln -s `pwd`/../../../qtiplot .
    ln -s `pwd`/../../../Mantid .
    cd ../..
else
    echo "Exporing mantid source revision $svn_rev from subversion ..."
    echo "Directory structure ..."
    svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Code/RPM_Kit/Mantid Mantid-$mantid_version 
#
    cd Mantid-$mantid_version
    echo "Images ..."
    svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Images Images
    echo "MantidPlot ..."
    svn -q --non-interactive export -r "$svn_rev" $mantid_svn/Code/qtiplot Code/qtiplot
    echo "src ..."
    svn -q --non-interactive export --force -r "$svn_rev" $mantid_svn/Code/Mantid Code/Mantid
    echo "instrument ..."
    svn -q --non-interactive export --force -r "$svn_rev" $mantid_svn/Test/Instrument instrument
    cd ..
fi
echo "Exporting complete - making spec file for version $mantid_version release $mantid_release"
#
sed -e "s/MANTID_VERSION/$mantid_version/" -e "s/MANTID_RELEASE/$mantid_release/" < mantid.spec.template > Mantid.spec
echo "Buliding tar file Mantid-$mantid_version.tar.gz"
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
rpmbuild -ba Mantid.spec
echo "Building complete - check $topdir/RPMS"
