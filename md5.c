#include <openssl/md5.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

#define DATA 1
#define HASH_PASS 2
#define RESULT 3
#define FINISH 4

static int dict[] = {'0','1','2','3','4','5','6','7','8','9', 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z', 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z', ' ','!','"','#','$','%','&','\'','(',')','*','+',',','-','.','/', ':',';','<','=','>','?','@','[','\\',']','^','_','`', '{','|','}','~'};

void convert2base(int base, int origin, int* s, int len)
{ // origin is restricted in range base^len
    if (origin > pow(base, len)) {
        printf("wrong value!\n");
        return;
    }
    int res[len];
    int tmp;
    int i = 0;
    while (1)
    {
        tmp = origin % base;
        origin /= base;
        res[i] = tmp;
        i++;
        // printf("thuong: %d, du: %d\n", origin, tmp);
        if (origin < base)
        {
            res[i] = origin;
            break;
        }
    }
    int j;
    for (j = i; j >= 0; j--)
    {
        s[len - j - 1] = res[j];
    }
}

void plusOne(int *position, int base, int arrlen)
{
    int tmp, i, memory = 0;
    // printf("base: %d, arrlen: %d\n", base, arrlen);
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
}

int rank0(char* processorname, char *hash_password, int num_process, int base, int passlen, int passtype, long sub_space, int offset, clock_t begin){
    long start = 0;
    int start_point[passlen];

    switch (passtype)
    {
        case 1: // number only
        {
            printf("Passtype: numbers only\n");
            break;
        }
        case 2: // lowercase characters only
        {
            printf("Passtype: lowercase characters only\n");
            break;
        }
        case 3: // lowercase & number characters
        {
            printf("Passtype: lowercase characters and numbers\n");
            break;
        }
        case 4: // lowercase & uppercase charactoers
        {
            printf("Passtype: lowercase & uppercase characters\n");
            break;
        }
        case 5: // number and character
        {
            printf("Passtype: numbers and characters\n");
            break;
        }
        case 6: // number, character, special character
        {
            printf("Passtype: numbers, characters and special characters\n");
            break;
        }
    }

    printf("P0-%s: passlen = %d, num_process = %d, sub_space = %ld, base = %d, offset = %d\n", processorname, passlen, num_process, sub_space, base, offset);
    long remain = pow(base, passlen) - sub_space * num_process;
    for (int i = 1; i < num_process; ++i){
        start = i*sub_space + remain;
        MPI_Send(&start, 1, MPI_LONG, i, DATA, MPI_COMM_WORLD);
    }
    // Init start point for frontend node
    
    for (int i = 0; i<passlen; ++i) {
        start_point[i] = 0;
    }

    long pos = 0;
    int isFound = 0, state = 0;
    unsigned char guess[passlen];
    unsigned char recv_guess[passlen];
    MPI_Request request;
    MPI_Request finish_request[num_process - 1];
    MPI_Status found_status;
    MPI_Status finish_status;
    int finished[num_process - 1];
    MPI_Irecv(recv_guess, passlen, MPI_CHAR, MPI_ANY_SOURCE, RESULT, MPI_COMM_WORLD, &request);
    
    while(pos < (sub_space + remain) && !isFound) {
        if (pos % 1000000) {
            printf("P0: checking...\n");
        }
        // find string by array of position in dict
        for (int i = 0; i < passlen; ++i){
            guess[i] = dict[start_point[i] + offset];
        }
        // hash with MD5
        unsigned char hash[MD5_DIGEST_LENGTH];
        unsigned char hash_hex[32];
        MD5(guess, strlen(guess), hash);
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i){
            sprintf(hash_hex + i*2, "%02x", (unsigned int)hash[i]);
        }
        // compare and return result
        if (strcmp(hash_hex, hash_password) == 0) {
            break;
        }
        pos ++;
        plusOne(start_point, base, passlen);
        MPI_Test(&request, &isFound, &found_status);
    }
    for (int i = 1; i < num_process; ++i) {
        MPI_Irecv(&state, 1, MPI_INT, i, FINISH, MPI_COMM_WORLD, finish_request + i - 1);
    }
    int isAllFinished = 0;
    while(num_process != 1 && (!isFound || !isAllFinished)) {
        for (int i = 1; i < num_process; ++i) {
            MPI_Test(&request, &isFound, &found_status);
            MPI_Test(finish_request + i - 1, finished + i - 1, &finish_status);
        }
        isAllFinished = 1;
        for (int i = 1; i < num_process; ++i) {
            if (!finished[i - 1]) {
                isAllFinished = 0;
                break;
            }
        }
    }

    if (isFound){
        printf("P0-%s: Password found: %s\n", processorname, recv_guess);
    } else if (pos < sub_space) {
        printf("P0-%s: Password found: %s\n", processorname, guess);
        for (int i = 0; i < num_process; ++i) {
            MPI_Send(&isFound, 1, MPI_INT, i, RESULT, MPI_COMM_WORLD);
        }
    } else {
        printf("something failed!\n");
    }
    
    printf("P0-%s: Process done!\n", processorname);
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("runtime: %fs\n", time_spent);
    return 0;
}
int ranki(int rank, char* processorname, int base, char* hash_password, int passlen, long sub_space, int offset){
    MPI_Status status;
    int start_point[passlen];
    for (int i = 0; i < passlen; ++i) start_point[i] = 0;

    long start;
    MPI_Recv(&start, 1, MPI_LONG, 0, DATA, MPI_COMM_WORLD, &status);
    convert2base(base, start, start_point, passlen);

    unsigned char guess[passlen];
    long found = -1, pos = 0;
    int isFound = 0, state = 0;
    MPI_Request request;
    MPI_Irecv(&found, 1, MPI_LONG, 0, RESULT, MPI_COMM_WORLD, &request);
    while(pos < sub_space && !isFound) {
        if (pos % 1000000) {
            printf("P%d: checking...\n", rank);
        }
        // find string by array of position in dict
        for (int i = 0; i < passlen; ++i){
            guess[i] = dict[start_point[i] + offset];
        }
        // hash with MD5
        unsigned char hash[MD5_DIGEST_LENGTH];
        unsigned char hash_hex[32];
        MD5(guess, passlen, hash);
        for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) 
            sprintf(&hash_hex[i*2], "%02x", (unsigned int)hash[i]);
        // compare and return result
        if (strcmp(hash_hex, hash_password) == 0) {
            MPI_Send(guess, passlen, MPI_CHAR, 0, RESULT, MPI_COMM_WORLD);
            break;
        }
        pos ++;
        plusOne(start_point, base, passlen);
        MPI_Test(&request, &isFound, &status);
    }
    MPI_Send(&state, 1, MPI_INT, 0, FINISH, MPI_COMM_WORLD);
    return 0;
}

int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);
    int rank, hostlen;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    char processorname[50];
    MPI_Get_processor_name(processorname, &hostlen);
    if (argc < 4) {
        if (rank == 0) printf("usage: md5 <input-password> <password-length> <password-type>[1/2/3/4/5]\n");
        MPI_Finalize();
        return 1;
    }
    clock_t begin = clock();

    char *hash_password = argv[1];
    int passlen = atoi(argv[2]);
    int passtype = atoi(argv[3]);
    int num_process = 0;
    MPI_Comm_size(MPI_COMM_WORLD, &num_process);
    int offset = 0, base = 10;
    switch (passtype)
    {
        case 1: // number only
        {
            base = 10;
            break;
        }
        case 2: // lowercase characters only
        {
            base = 26;
            offset = 10;
            break;
        }
        case 3: // lowercase & number characters
        {
            base = 36;
            break;
        }
        case 4: // lowercase & uppercase characters
        {
            base = 52;
            offset = 10;
            break;
        }
        case 5: // number and characters
        {
            base = 62;
            break;
        }
        case 6: // number, character, special character
        {
            base = 95;
            break;
        }
    }

    long sub_space = (long)pow(base, passlen) / num_process;
    if (rank == 0) {
        rank0(processorname, hash_password, num_process, base, passlen, passtype, sub_space, offset, begin);
    } else {
        ranki(rank, processorname, base, hash_password, passlen, sub_space, offset);
    }

    printf("Process %d exiting...\n", rank);
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

/* 
int **partition(int base, int num_process, long sub_space, int passlen)
{
    int **partition_point = (int **)malloc(num_process * sizeof(int *));
    for (int i = 0; i < num_process; ++i)
    {
        partition_point[i] = (int *)malloc(passlen * sizeof(int));
    }

    printf("Start partition: base-%d, num_process-%d, sub_space-%ld, passlen-%d\n", base, num_process, sub_space, passlen);
    printf("size partition_point: %ld\n", sizeof(partition_point[0]));
    int start = 0;
    for (int i = 0; i < num_process; ++i)
    {
        printf("start: %d\n", start);
        for (int j = 0; j < passlen; ++j)
        {
            partition_point[i][j] = 0;
        }

        convert2base(base, start, partition_point[i], passlen);
        start += sub_space;
    }
    return partition_point;
}

int hashcmp(char* hasha, char* hashb) {
    int size = sizeof(hasha) / sizeof(hasha[0]);
    for (int i = 0; i < size; ++i){
        if (hasha[i] < hashb[i]) {
            return -1;
        } else if (hasha[i] > hashb[i]){
            return 1;
        }
    }
    return 0;
}
*/