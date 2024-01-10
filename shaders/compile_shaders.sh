#!/bin/bash
for shader in `ls *.vert` 
do
    echo Compiling $shader
    filename=$(basename -- "shader")
    extension="${filename##*.}"
    filename="${filename%.*}"
    glslc $shader -o `echo $filename`_vert.spv && echo done
done

for shader in `ls *.frag` 
do
    echo Compiling $shader
    filename=$(basename -- "$shader")
    extension="${filename##*.}"
    filename="${filename%.*}"
    glslc $shader -o `echo $filename`_frag.spv && echo done
done