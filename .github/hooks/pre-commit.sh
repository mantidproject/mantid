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
      # return zero if files were changed - copilot will see those changes
      exit 0
    else
      # return pre-commit's result if no files were changed
      exit ${rc}
    fi
  fi
fi
