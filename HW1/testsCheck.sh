#!/bin/bash

# Loop through 99 iterations
for i in {1..99}; do
  
  # Run bp_main with input example$i and save output to test$i
  ./bp_main example$i.trc > test$i.txt

done