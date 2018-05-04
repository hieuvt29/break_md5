### Brute-force algorithm to break MD5 (Message-Digest algorithm 5)

1. Prerequisites
- MPI
- Rocks clusters (if we want to run across many machine)
1. Compile
`$ mpicc -o md5 md5.c -lm -lcrypto`

2. Run
`$ mpirun -np <num_process> [-hostfile <hostfile>] md5 <md5 hashed pass> <pass length> <pass type>`
