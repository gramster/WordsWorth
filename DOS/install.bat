rem Specify the source and destination drives and destination directory
rem For example "INSTALL A: C: \WW"

@echo off
%2
chdir \
mkdir %3
chdir %3
%1unzip %1wwship

