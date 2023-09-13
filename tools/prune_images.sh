#!/bin/bash
IMAGE_DIRECTORY=$PWD"/docs/source/images"


if [ "$1" = "--dry-run" ]; then
    delete_file() {
        echo \'$1$2\' would be deleted, but this is a dry-run
    }
else
    delete_file() {
        echo \'$1$2\' has been deleted
    }
fi


remove_unused_images() {
    extension=$1
    while IFS= read -r -d $'\0' filepath; do
        filename=$(basename "$filepath" $extension)
        count=$(git grep -o $filename | wc -l)
	if [ $count -eq 0 ]; then
	    delete_file $filename $extension
	fi
    done < <(find "$IMAGE_DIRECTORY" -type f -name "*$extension" -print0)
}


remove_unused_images ".png"
remove_unused_images ".PNG"
remove_unused_images ".jpeg"

