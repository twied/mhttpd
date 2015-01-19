#!/bin/sh
mkdir -p m4
autoreconf --force -v --install && ./configure
