#
# Make file to
#
#
UIFILES := $(wildcard *.ui)
PYFILES := $(UIFILES:.ui=.py)

.PHONY: all
all: $(PYFILES)

%.py: %.ui
	pyuic4 $< --output $@

clean: 
	rm *pyc */*.pyc
