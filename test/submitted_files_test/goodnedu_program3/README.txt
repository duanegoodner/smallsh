INSTRUCTIONS FOR COMPILING SOURCE CODE:

Step 1. Save all of the following files in the same directory:
    
    makefile

    main.c
    command.c
    built_ins.c
    process_mgmt.c
    signal_handling.c
    utilities.c

    globals.h
    command.h
    built_ins.h
    process_mgmt.h
    signal_handling.h
    utilities.h
  
  

Step 2. cd into directory where above files are saved, and enter the following command at the command prompt:
    
    $ make
    
    (Note: The "$" is NOT part of the command.)


The above procedure will produce an executable file called smallsh that can be run be run using the command:

    $ ./smallsh
