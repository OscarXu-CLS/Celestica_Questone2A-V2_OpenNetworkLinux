###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_ds2000_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_ds2000_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_ds2000_DEPENDMODULE_ENTRIES := init:x86_64_cel_ds2000 ucli:x86_64_cel_ds2000

