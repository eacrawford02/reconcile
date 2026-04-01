#!/usr/local/bin/bash

if [ -z "$1" ]; then
  echo "Usage: ./dos_to_unix.sh file [file ...]"
  exit 1
fi

for arg in "$@"; do
  case $arg in
    -h*)
      ;&
    --help*)
      echo "Usage: ./dos_to_unix.sh file [file ...]"
      exit 1
      ;;
  esac
done

for file in "$@"; do
  if [[ -f "$file" ]]; then
    sed 's///g' "$file" > "$file.tmp" && mv "$file.tmp" "$file"
  fi
done
