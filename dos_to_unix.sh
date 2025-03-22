#!/usr/local/bin/bash

for file in *.csv; do
  if [[ -f "$file" ]]; then
    sed 's///g' "$file" > "$file.tmp" && mv "$file.tmp" "$file"
  fi
done
