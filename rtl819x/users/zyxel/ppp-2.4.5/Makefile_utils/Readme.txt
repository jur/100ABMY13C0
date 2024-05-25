cp Makefile ppp-2.4.5/pppd/Makefile

The Makefile is for compiling finish that is success.
Because using ./configure is during compiling status that will base on the environment states to create Makefile
and compile will fail, that includes some libs and files but the toolchain does not support them.
