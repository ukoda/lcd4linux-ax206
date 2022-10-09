#!/bin/sh

touch lcd4linux.c
echo "#define VCS_VERSION \"`git describe --tags`\"" > vcs_version.h

