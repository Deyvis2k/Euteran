#!/bin/bash

#set the base dir
current_dir="$PWD"
find "$current_dir/temp/symlinks" -type l | while read link; do
  if [ ! -e "$(readlink "$link")" ]; then
    rm "$link"
  fi
done
