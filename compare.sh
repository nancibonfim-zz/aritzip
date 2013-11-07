#!/bin/bash

diff -s -q  <(md5sum $1 | cut -b-32) <(md5sum $2 | cut -b-32)
exit 0
