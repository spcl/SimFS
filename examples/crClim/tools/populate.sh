#!/bin/bash

ctxname=$1
for f in $(simfs $ctxname ls | grep "lff" | cut -d$'\t' -f 1); do touch "$f"; done;
