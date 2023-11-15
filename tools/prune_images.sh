#!/bin/bash
#
# Description: This script can be used to prune the images stored in the /docs/source/images directory.
#              It will delete any images which are not referenced anywhere else in the Mantid repo.
#
# Usage: ./tools/prune_images.sh --dry-run
IMAGE_DIRECTORY=$PWD"/docs/source/images"


if [ "$1" = "--dry-run" ]; then
    delete_file() {
        echo \'$1\' would be deleted, but this is a dry-run
    }
else
    delete_file() {
	find $IMAGE_DIRECTORY -type f -name "$1" -exec rm {} \;
        echo \'$1\' has been deleted
    }
fi


remove_unused_images() {
    extension=$1
    while IFS= read -r -d $'\0' filepath; do
        filename=$(basename "$filepath")
        count=$(git grep -o $filename | wc -l)
	if [ $count -eq 0 ]; then
	    delete_file $filename
	fi
    done < <(find "$IMAGE_DIRECTORY" -type f -name "*$extension" -print0)
}


remove_unused_images ".png"
remove_unused_images ".PNG"
remove_unused_images ".jpeg"

