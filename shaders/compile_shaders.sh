#!/bin/bash
for shader in `ls *.vert` 
do 
    glslc $shader -o $shader_vert.spv 
done

for shader in `ls *.frag` 
do
    glslc $shader -o $shader_frag.spv 
done