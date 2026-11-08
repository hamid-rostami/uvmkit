#!/usr/bin/env bash

guix time-machine -C guix/channels.scm -- shell --container \
     --share=$PWD -m guix/manifest.scm
