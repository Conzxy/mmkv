#!/bin/bash

PrintHelp() {
  echo "Usage: ./build.sh target_name [--mode|-m] [-v|--verbose]"
  echo "Options: "
  echo "-m|--mode=debug/release  Build mode(Case insensitive)"
  echo "-v|--verbose             Print detail message"
  exit 0
}

CheckTargetIsSet() {
  if [[ -z "$TARGET" ]]; then
    echo "The build target must place in the first argument"
    exit 1
  fi
}

if [ $# -lt 1 ]; then
  PrintHelp
  exit 1
fi

case "$1" in
  -h|--help):
    PrintHelp
  ;;
  
  --*):
  ;& # continue execute the list associated with the next pattern
  -?):
    echo "Invalid first argument"
    PrintHelp
    exit 1
  ;;
  *):
    TARGET="$1"
  ;;
esac

shift

MODE="Debug"
VERBOSE=0

CheckTargetIsSet

for arg in "$@"; do
  case "$arg" in
    -m=*|--mode=*):
      MODE="${arg#*=}"
    ;;
    -v|--verbose):
      VERBOSE=1
    ;;
    -h|--help):
      PrintHelp
    ;;
    *):
      echo "Unknown option, don't accpet" 
      exit 1;
    ;;
  esac
done

[[ ${MODE,,} == "debug" ]] && MODE="Debug" || MODE="Release"
[[ $VERBOSE == 1 ]] && VERBOSE="-v" || VERBOSE=""

if [[ -z "$MMKV_BUILD_PATH" ]]; then
  echo "You must export the build path of mmkv"
  echo "e.g. export MMKV_BUILD_PATH=..."
  echo "OR append \"export MMKV_BUILD_PATH=...\" to your .bash_profile or .zshrc, etc."
  exit 1
fi

cd $MMKV_BUILD_PATH
cmake .. -DCMAKE_BUILD_TYPE=$MODE
cmake --build . --target $TARGET --parallel $(nproc) $VERBOSE
