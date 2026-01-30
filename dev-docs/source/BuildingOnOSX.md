# Building on OS X

The minimum supported version of macOS is High Sierra (10.13).

These instructions are from the assumptions of a blank newly installed
version of High Sierra using Homebrew for dependency management.

1.  Install Xcode 10.1
2.  Install Apple's Command Line tools (required by Homebrew)

``` sh
xcode-select --install
```

3.  Install [Homebrew](http://brew.sh).

``` sh
ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
```

4.  Add the necessary 'taps':

In order to be able to 'tap' the `mantidproject/mantid` 'tap' we need to
have a couple of packages installed

``` sh
brew install git
brew tap mantidproject/mantid
brew tap homebrew/cask
brew cask install xquartz
brew cask install mactex
```

5.  Install `mantid-developer` formula (this may take a while depending
    on your network speed)

``` sh
brew install mantid-developer
```

6.  Homebrew can stop early for reasons that are unclear. Repeat the
    above command until Homebrew states: *Warning:
    mantidproject/mantid/mantid-developer ?.? is already installed and
    up-to-date*.
7.  Unlink `qscintilla2`

``` sh
brew unlink qscintilla2
```

8.  Python is now keg-only. Add the appropriate version to `PATH` in
    shell profile and restart the terminal:

``` sh
# Assume we are using bash
echo 'export PATH="/usr/local/opt/python@3.8/bin:$PATH"' >> ~/.bash_profile

# If you have enabled Zsh
echo 'export PATH="/usr/local/opt/python@3.8/bin:$PATH"' >> ~/.zshenv
```

9.  Downgrade setuptools to 48.0.0 until
    <https://github.com/mantidproject/mantid/issues/29010> is fixed.

``` sh
python3 -m pip install setuptools==48.0.0
```

10. Install python requirements

``` sh
python3 -m pip install -r /usr/local/Homebrew/Library/Taps/mantidproject/homebrew-mantid/requirements.txt
```
