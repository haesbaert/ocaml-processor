#!/bin/sh

scp ioreg_apple.c macmini: && \
	ssh macmini cc -o ioreg_apple ioreg_apple.c -Wall \
	    -framework IOKit -framework Foundation && \
	ssh macmini ./ioreg_apple
