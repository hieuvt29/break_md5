### Brute-force algorithm to break MD5 (Message-Digest algorithm 5)

1. Prerequisites
- MPI
- Rocks clusters (if we want to run across many machine)
1. Compile  
`$ mpicc [-std=c99] -o md5 md5.c -lm -lcrypto`

2. Run  
`$ mpirun -np <num_process> [-hostfile <hostfile>] md5 <md5 hashed pass> <pass length> <pass type>`
- `<hostfile>` contains name of hosts you want to run processes on, each name in a separate row
- `<md5 hashed pass>` is the message after hashing by md5, we can get that by `$ echo -n "<message>" | md5sum`
