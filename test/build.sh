#!/bin/bash
if [ "$#" != "1" ]; 
then
  echo "Usage: ./build.sh target_name(also can be used for extension, e.g. *.cc)"
  exit 1
fi

target_name="$(echo $1 | cut -f 1 -d '.')"

echo "target_name: $target_name"
echo

cd ~/mmkv/build
cmake .. -DCMAKE_BUILD_TYPE=Debug &&
  cmake --build . --target $target_name
