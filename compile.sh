#!/bin/bash

echo $(gcc -o inicjator Inicjator.c -lpthread)
echo $(gcc -o P1 Proces1.c -lpthread)
echo $(gcc -o P2 Proces2.c -lpthread)
echo $(gcc -o P3 Proces3.c -lpthread)
