#!/bin/bash 
function usage {
echo " 
Usage: $0 [OPTION]... <LIBGIT2>...
Copy the minimum files from LIBGIT2 repository, that are necessary to build it inside Mantid. 

Mantid will link to libgit2, but, as long libgit2 is a quite active project, snapshots will 
be taken from the repository, that are known to operate correctly for the current release of 
Mantid, specially, the GitRepositorySharing module. 

This scripts copy the files that are required to build Mantid using libgit2. 

"
}


if [ "x$1" == "x" ];then
  usage
  exit
fi;

#create libgit2 folder if it does not exists
mkdir -p libgit2
cp -r $1"/deps" $1"/src" $1"/include" $1"/CMakeLists.txt" "$1/libgit2.pc.in" libgit2
if [ $? != '0' ]; then
  echo "Invalid libgit2 repository folder"
  usage
  exit
fi;

cd libgit2
#some adjustments to CMakeLists.txt
#switch off clar
sed -i 's/OPTION (BUILD_CLAR.*/\#&/g' CMakeLists.txt
#switch off build examples
sed -i 's/OPTION (BUILD_EXAMPLES.*/\#&/g' CMakeLists.txt

#switch off tab
sed -i 's/OPTION (TAGS.*/\#&/g' CMakeLists.txt

#switch off profile
sed -i 's/OPTION (PROFILE.*/\#&/g' CMakeLists.txt

cd ..

