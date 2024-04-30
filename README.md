# Producer-Consumer-Encoder
A multi-process application demonstrating synchronous inter-process communication using a producer-consumer scheme. This project consists of three main components:

-> Producer (Process 1): Prompts the user for a directory path and then traverses the directory to pass complete paths of all resources (files, subdirectories) to the consumer through the pipe communication mechanism.

-> Encoder (Process 2): Receives paths from the producer, encodes each path into a hexadecimal representation, and prints both the original and encoded paths on the screen. It then sends the encoded paths to the writer process via the shared memory communication mechanism.

-> Writer (Process 3): Collects the encoded data from the encoder process and displays it, showcasing the end-to-end process of reading, encoding, and outputting resource paths.

Additionally, a user can send signals to each of the processes to initiate a specific action. Possible signals are:

-> SIGIO: Ends a program, releasing all resources.

-> SIGPWR: Pauses a program (stops data transmission).

-> SIGSYS: Resumes a program (enables data transmission).

-> SIGRTMIN: Turns ON/OFF data encryption between Process 1 and Process 2.
  
This program is using a semaphore and mutex as a tool of managing all processes.
Note that this code is for showcasing a solution to a problem of managing 3 processes, running it correctly requires creating specific names of the files and saving them on designated path.
