/* simple send and receive */ 
#include <stdio.h>
#include <openssl/md5.h>

int main (int argc, char **argv) {

    int passlen = 4;
    unsigned char guess[] = "aaaa";

    unsigned char hash[MD5_DIGEST_LENGTH];
    
    MD5(guess, passlen, hash);
    for (int i = 0; i < 16; ++i) {
        printf("%02x", (unsigned int) hash[i]);
    }
    printf("\n%s\n", guess);
    return 0;
}