changed_files=$(git diff --name-only)
if [ -n "${changed_files}" ]; then
  # run pre-commit and capture the return
  pre-commit run --files ${changed_files}; rc=$?
  # do something special if nonzero return
  if [ ${rc} -ne 0 ]; then
    if git diff --quiet; then
      # return pre-commit's result if no files were changed
      exit ${rc}
    else
      # return zero if files were changed - copilot will see those changes
      exit 0
    fi
  fi
fi
