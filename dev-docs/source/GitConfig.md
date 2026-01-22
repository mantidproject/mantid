# Mantid Git Config

::: {.contents local=""}
Contents
:::

## Summary

This page describes some configuration that will help with your usage of
Git - some of which is strongly recommended. This should all be added to
the `.gitconfig` file in your user home directory, i.e. `~/` on Linux or
`%USERPROFILE%` on Windows. This will then be global across all of your
repositories - please sanity check these suggestions if you use git for
other repositories.

Note that some of the values are commented out because they must be
updated with your own details or are OS specific. Some commands use
third party tools which may need to be installed.

## Recommended Config

It is recommended to set up the following as a minimum:

``` sh
[user]
        # Uncomment and set these to your own details
        #name =
        #email =
[core]
        # Uncomment and set an editor you like. If you're not sure, try Nano on Linux or Notepad++ on Windows
        #editor = nano
        #editor = "'C:/Program Files (x86)/Notepad++/notepad++.exe' -multiInst -notabbar -nosession -noPlugin"
        # Specify your own global list of files/patterns that git should ignore
        excludesfile = ~/.gitignore
        # Fix up line endings between windows/linux - uncomment one
        # Linux
        #autocrlf = input
        # Windows
        #autocrlf = true
[log]
        # Get the log to follow files even if they were renamed
        follow = true
[fetch]
        # Remove remote-tracking references that no longer exist on the remote when you do a fetch/pull
        prune = true
[push]
        # The safest default for git-push: push only the current branch and only if it has the same name as upstream
        default = simple
[pull]
        # Only pull the current branch by default
        default = current
        # Only allow pull if it can be fast-forwarded (otherwise you explicitly need to merge in the remote or rebase onto it e.g. with git pull --rebase)
        ff = only
[diff]
        # Add extra colours for highlighting moved code
        colorMoved = default
[merge]
        # Include the common ancestor in conflict details - very useful for working out which changes to keep
        conflictstyle = diff3
[rebase]
        # Automatically reposition !fixup/!squash commits in interactive rebase (useful with git commit --fixup/squash)
        autosquash = true
```

## Diff/Merge Tools

You may find the following diff/merge tools helpful:

``` sh
# Uncomment lines where appropriate for specific OS
[diff]
        # Set up a graphical tool for viewing diffs. I like meld because it looks good and
        # does a good job with directory diffs
        tool = meld
[difftool]
        # Disable the prompt when opening the difftool
        prompt = false
[merge]
        # Specify a graphical tool for resolving merge conflicts. I like diffmerge because you
        # can toggle between the merged file and common ancestor
        tool = diffmerge
[difftool "meld"]
        external = meld
        # Windows
        #cmd = "'C:/Program Files (x86)/Meld/Meld.exe' $LOCAL $REMOTE"
        #path = C:/Program Files (x86)/Meld/Meld.exe
[mergetool "meld"]
        trustExitCode = true
        keepBackup = false
        # Windows
        #cmd = "'C:/Program Files (x86)/Meld/Meld.exe' $LOCAL $BASE $REMOTE --output=$MERGED"
        #path = C:/Program Files (x86)/Meld/Meld.exe
[difftool "diffmerge"]
        # Linux
        #cmd = diffmerge $LOCAL $REMOTE
        # Windows
        #cmd = "'C:/Program Files/SourceGear/common/DiffMerge/sgdm.exe' $LOCAL $REMOTE"
[mergetool "diffmerge"]
        trustExitCode = true
        keepBackup = false
        # Linux
        #cmd = diffmerge -merge -result="$MERGED" "$LOCAL" "$BASE" "$REMOTE"
        # Windows
        #cmd = "'C:/Program Files/SourceGear/common/DiffMerge/sgdm.exe' -merge -result=$MERGED $LOCAL $BASE $REMOTE"
```

## Useful Aliases

You may find the following aliases helpful. You can also add your own.

``` sh
[pretty]
        concise = "%C(yellow)%h%Creset %C(green)[%ar]%Creset %C(auto)%d%Creset %s %C(cyan)<%an>"
        detail = "%C(yellow)commit %h%Creset%C(auto)%d%Creset%n%C(yellow)Parents: %p%Creset%n%C(cyan)Author: %an <%ae>%Creset%n%C(cyan)        %ai (%ar)%Creset%n%C(green)Commit: %cn <%ce>%Creset%n%C(green)        %ci (%cr)%Creset%n%n%w(79)%s%n%n%b"

[alias]
        # Check out a pull request for testing. The first argument is the name of the remote and the
        # second is the pull request number,
        # e.g. git test-pr origin 12345
        test-pr = "!f() { git fetch $1 pull/$2/merge:pr/$2-merged && git checkout pr/$2-merged; }; f"
        # Remove all branches starting pr/
        test-pr-remove-all = "!f() { git branch | grep pr/ | xargs git branch -D; }; f"

        # Get into the habit of using add --patch to create focused commits
        ap = add --patch
        # Amend the latest commit
        ca = commit --amend
        # Amend the latest commit reuse commit message from latest commit
        cah = commit --amend --reuse-message=HEAD

        # Pretty oneline log with a bit more info than git log --oneline
        lg = log --pretty=concise
        # Pretty log showing as a graph
        lgg = log --pretty=concise --graph
        # Pretty log showing only the commits on the current branch i.e. since main
        lgm = log --pretty=concise main..
        # Show all of the files changed on the current branch i.e. since main
        lfc = "!f() { git log --name-only --format= main.. | sort | uniq;  }; f"

        # Show commit details with file names only
        sh = show --stat --pretty=detail
        # Show commit details including diffs (same as git-show but slightly prettier)
        shd = show --pretty=detail

        # A more concise output from status, also showing which branch you're on
        st = status --short --branch
        # A more verbose output from branch, showing the commit and upstream branch
        br = branch -vv

        # When pushing a new branch upstream, set the local branch to track it
        pushu = push --set-upstream
        # Force push with lease (safer than push --force, but still be careful when changing history)
        pushfl = push --force-with-lease
```

## More Advanced Options

The following may be useful if you get into more in-depth usage of Git:

``` sh
[rerere]
        # Reuse Recorded Resolutions - useful if you find yourself fixing the same conflicts over and over
        enabled = true
        # Auto-stage files resolved by rerere
        autoupdate = true
[blame]
        # This file allows you to specify commits that should be ignored in git-blame e.g. bulk changes
        ignoreRevsFile = ~/.git-blame-ignore-revs
[include]
        # You can split your config into sub-files e.g. to include common config on different OS's
        path = ~/.gitconfig_common
```
