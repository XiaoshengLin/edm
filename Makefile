# Makefile for medm top level

TOP = ../..
ifneq ($(wildcard $(TOP)/config)x,x)
  # New Makefile.Host config file location
  include $(TOP)/config/CONFIG_EXTENSIONS
  DIRS = lib baselib edmMain pvlib imagelib
  include $(TOP)/config/RULES_DIRS
else
  # Old Makefile.Unix config file location
  EPICS=../../..
  include $(EPICS)/config/CONFIG_EXTENSIONS
  DIRS = lib baselib edmMain pvlib imagelib 
  include $(EPICS)/config/RULES_DIRS
endif
