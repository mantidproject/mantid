#! /bin/bash

# dmgpack diskname [file|dir]+
#
#    Copy a group of files/directories to a compressed disk image.
#    The resulting image is stored in diskname.dmg 
#
#    Files are copied with 'ditto' to preserve resource forks.  For
#    convenience we also call FixupResourceForks after copying.  This
#    allows you to use /Developer/Tools/SplitFork on your tree and 
#    manipulate it with CVS, tar, etc.  Don't forget the -kb option 
#    when adding or committing app and ._app files to CVS!
#
#    This command will fail if a volume of the given name is already
#    mounted.  It could also fail if the size of the resource forks
#    is large compared to the size of the data forks. Change the
#    scale factor internally from 11/10 to a more appropriate number
#    if it complains it is running out of space.
#
#    It is possible to add a license agreement to a dmg file.  See
#    the "Software License Agreements for UDIFs" sdk available at
#    http://developer.apple.com/sdk/index.html

test $# -lt 2 && echo "usage: $0 diskname [file|dir]+" && exit 1
#set -x
NAME="${1%.dmg}" ; shift
DISK=/tmp/dmgpack$$.dmg
COMPRESSED="$NAME.dmg"
VOLUME="$NAME"

# compute needed image size; scale it by 10%
SIZE=$(du -ck "$@" | tail -1 | sed -e 's/ *total//')
SIZE=$(echo $SIZE*11/10 | bc)
test $SIZE -lt 4200 && SIZE=4200

# create the disk
rm -f $DISK
hdiutil create -size ${SIZE}k $DISK -layout NONE

# create a file system on the disk; last line of output is
# the device on which the disk was attached.
DEVICE=$(hdiutil attach $DISK -nomount | tail -1)
newfs_hfs -v "$VOLUME" $DEVICE

# mount the file system
mkdir $DISK-mount
mount -t hfs $DEVICE $DISK-mount || (echo "mount $DISK-mount failed" && exit 1)

# copy stuff to the disk and fixup resource forks
for f in "$@"; do 
    f=${f%/}		;# strip trailing /
    dest="$DISK-mount/${f##*/}"
    ditto -rsrc "$f" "$dest" 
    test -d "$f" && /System/Library/CoreServices/FixupResourceForks "$dest"
done

# eject the disk
umount $DISK-mount
rmdir $DISK-mount
hdiutil eject $DEVICE

# compress the disk and make it read only
rm -f "$COMPRESSED"
hdiutil convert -format UDZO $DISK -imagekey zlib-level=9 -o "$COMPRESSED"
rm -f $DISK
