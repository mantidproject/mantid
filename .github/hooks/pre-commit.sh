#!/usr/bin/bash

# changed files that aren't deleted as a bash array
changed_files=( $(git diff --diff-filter=d --name-only) )

# only if there are files changed
if [ "${#changed_files[@]}" -ne 0 ]; then
  # run pre-commit and capture the return
  pre-commit run --files "${changed_files[@]}"; rc=$?
  # do something special if nonzero return
  if [ ${rc} -ne 0 ]; then
    # this if is intentionally backwards
    if ! git diff --quiet; then
      # run pre-commit a second time to allow for pre-commit fixing things
      # on the first pass
      pre-commit run --files "${changed_files[@]}"; rc=$?
    fi
  fi
  # return pre-commit's result
  exit ${rc}
fi

# exit that it is fine since it got this far
exit 0
