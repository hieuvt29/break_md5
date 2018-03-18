#include <openssl/md5.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>

#define DATA 1
#define RESULT 2

#define dict {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', ':', ';', '<', '=', '>', '?', '@', '[', '\\', ']', '^', '_', '`', '{', '|', '}', '~'};

void convert2base(int n, int origin, int *s)
{ // n = 2, origin = 1 => 0001
    int res[100];
    int tmp;
    int i = 0;
    while (1)
    {
        tmp = origin % n;
        origin /= n;
        res[i] = tmp;
        i++;
        // printf("thuong: %d, du: %d\n", origin, tmp);
        if (origin < n)
        {
            res[i] = origin;
            break;
        }
    }
    int j;
    int slen = sizeof(s) / sizeof(s[0]);
    for (j = i; j >= 0; j--)
    {
        s[slen - j] = res[j];
    }
}

int **partition(int base, int num_process, long sub_space, int passlen)
{
    int **partition_point = (int **)malloc(num_process * sizeof(int *));
    for (int i = 0; i < num_process; ++i)
    {
        partition_point[i] = (int *)malloc(passlen * sizeof(int));
    }

    printf("Start partition: base-%d, num_process-%d, sub_space-%ld, passlen-%d\n", base, num_process, sub_space, passlen);
    printf("size partition_point: %d\n", sizeof(partition_point[0]));
    int start = 0;
    for (int i = 0; i < num_process; ++i)
    {
        printf("start: %d\n", start);
        for (int j = 0; j < passlen; ++j)
        {
            partition_point[i][j] = 0;
        }
        convert2base(base, start, partition_point[i]);
        start += sub_space;
    }
    return partition_point;
}

int *plusOne(int *position, int base, int arrlen)
{
    int tmp, i, memory = 0;
    printf("base: %d, arrlen: %d\n", base, arrlen);
    tmp = position[arrlen - 1] + 1;
    if (tmp < base)
    {
        position[arrlen - 1] = tmp;
    }
    else
    {
        position[arrlen - 1] = base - tmp;
        memory = 1;
        i = arrlen - 2;
        do
        {
            tmp = position[i] + memory;
            if (tmp < base)
            {
                position[i] = tmp;
                memory = 0;
            }
            else
            {
                position[i] = base - tmp;
                memory = 1;
            }
            i -= 1;
        } while (memory && i >= 0);
    }
    return position;
}
long process(int* start_point, int passlen, int base, int offset) {
    long pos = 0;
    int* next_position = start_point;
    char guess[passlen];
    do {
        // find string by array of position in dict
        for (int i = 0; i < passlen; ++i){
            guess[i] = dict[next_position[i] + offset];
        }
        // hash with MD5
        char hash[MD5_DIGEST_LENGTH];
        MD5(guess, passlen, hash);
        // compare and return result
        if (strcmp(guess, hash_password)) {
            break;
        }
        pos ++;
        next_position = plusOne(next_position, base, passlen);
    } while(pos < sub_space);
    return (pos >= sub_space)?-1:pos;
}

int rank0(char *hash_password, int num_process, int base, int passlen, long sub_space, int offset){
    // start position of subspace for each process
    int **partition_point = partition(base, num_process, sub_space, passlen);

    MPI_Status status;
    int rank = 0;
    for (int i = 1; i < num_process; ++i){
        // MPI_Send(&hash_password, passlen, MPI_CHAR, i, DATA, MPI_COMM_WORLD);
        MPI_Send(partition_point + i*passlen, passlen, MPI_INT, i, DATA, MPI_COMM_WORLD);
    }
    long res = process(partition_point[0], passlen, base, offset);
    if (res != -1) {
        printf("password found in process 0, at position: %ld!\n", res);
    } else {
        long found;
        for (int i = 1; i < num_process; ++i){
            MPI_Recv(&found, 1, MPI_INT, i, RESULT, MPI_COMM_WORLD, &status);
            if (found != -1) {
                printf("password found in process %d, at position: %ld!\n", i, found);
                break;
            }
        }
    }
    
    printf("Free partition_point...\n");
    for (int i = 0; i < num_process; ++i)
    {
        free(partition_point[i]);
    }
    return 0;
}
int ranki(char *hash_password, int num_process, int base, int passlen, long sub_space, int offset){
    MPI_status status;
    char start_point[passlen];
    MPI_Recv(start_point, passlen, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);
    long res = process(start_point, passlen, base, offset);
    MPI_Send(&res, 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD);
    return 0;
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    char *password = argv[1];
    int passlen = atoi(argv[2]);
    int passtype = atoi(argv[3]);
    int num_process = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
    long sub_space = (long)pow(base, passlen) / num_process;

    int offset = 0, base = 0;
    switch (passtype)
    {
        case 1: // number only
        {
            printf("Passtype: number only\n");
            base = 10;
            break;
        }
        case 2: // character only
        {
            printf("Passtype: character only\n");
            base = 52;
            offset = 10;
            break;
        }
        case 3: // number and character
        {
            printf("Passtype: number and character\n");
            base = 62;
            break;
        }
        case 4: // number, character, special character
        {
            printf("Passtype: number, character and special character\n");
            base = 95;
            break;
        }
    }
    printf("passlen: %d, num_process: %d, sub_space: %d, base: %d, offset: %d\n", passlen, num_process, sub_space, base, offset);

    char hash_password[MD5_DIGEST_LENGTH];
    MD5(password, passlen, hash_password);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        rank0(hash_password, num_process, base, passlen, sub_space, offset);
    } else {
        ranki(hash_password, num_process, base, passlen, sub_space, offset);
    }

    printf("\nprocess %d exiting...\n", rank);
    MPI_Finalize();
    return 0;
}
/*
- số ký tự: N 
1. [32 - 47]:  Khoảng trống (␠) ! " # $ % & ' ( ) * + , - . /
2. [48-57]: 0-9
3. [58-64]: : ; < = > ? @
4. [65-90]: A-Z
5. [91-96]: [ \ ] ^ _ `
6. [97-122]: a-z
7. [123-126]: { | } ~
- tập ký tự: chỉ số (2)/ chỉ chữ (4,6)/số và chữ (2,4,6)/ số và chữ và ký tự đặc biệt (1-7)

["0","1","2","3","4","5","6","7","8","9", "a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s","t","u","v","w","x","y","z", "A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z", " ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".","/", ":",";","<","=",">","?","@","[","\\","]","^","_","`", "{","|","}","~"]
- tập ký tự: chỉ số (0-9)/ chỉ chữ (10-61)/số và chữ (0-61)/ số và chữ và ký tự đặc biệt (0-94)
*/