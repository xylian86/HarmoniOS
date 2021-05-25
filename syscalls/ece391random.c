#include <stdint.h>
#include "ece391support.h"
#include "ece391syscall.h"


static uint32_t prngState = 0;

uint32_t init_rand(uint32_t seed)
{
    // set generator based on pseudo-random number
    prngState += seed;
    return 0;
}

uint32_t _rand(void)
{
    uint32_t value;

    prngState *= 1103515245;
    prngState += 12345;
    value = (prngState >> 16) & 0x07FF;

    prngState *= 1103515245;
    prngState += 12345;
    value <<= 10;
    value |= (prngState >> 16) & 0x03FF;

    prngState *= 1103515245;
    prngState += 12345;
    value <<= 10;
    value |= (prngState >> 16) & 0x03FF;

    return value;
}


int32_t rand_range(int32_t min, int32_t max)
{
    int value;

    // valid parameters.
    if(max>min)
    {
        // pick up a random value in the given range
        value = min + (_rand() % (max-min+1));
    }else{
        value = min;
    }


    return value;
}

int main()
{
    int i=0;

    // char buf[22];

    uint8_t arg_base[1024];
    uint8_t arg_seq[1024];
    uint8_t arg_game[1024];
    
    int buflist[10];

    int check;
    int seed;
    int running;
    int guess_result;
    int input_value;

    int32_t length;

    for(i=0;i<1024;i++)
    {
        arg_base[i] = (uint8_t) '\0';
        arg_seq[i] = (uint8_t)'\0';
        arg_game[i] = (uint8_t)'\0';
    }

    if (0 != ece391_random()) {
        ece391_fdputs (1, (uint8_t*)"Running random command failed\n");
        return 3;
    }

    ece391_fdputs (1, (uint8_t*)"Please Input Seed from 0-9:\n");

    check = ece391_read(0, arg_base, 1023);
    if(check==-1)
    {
        ece391_fdputs (1, (uint8_t*)"Please Input Correct Argument!\n");
        return 2;       
    }
    // get seed;
    seed = (arg_base[0]-'0');

    if((seed<0)||(seed>9))
    {
        ece391_fdputs (1, (uint8_t*)"Please Input Seed from 0-9!\n");
        return 2;       
    }

    ece391_fdputs (1, (uint8_t*)"Start the Guessing Game from 0 to 100!\n");
    init_rand(seed);

    ece391_fdputs (1, (uint8_t*)"Please input random sequence number (0-9):\n");
    check = ece391_read(0, arg_seq, 1023);
    if(check==-1)
    {
        ece391_fdputs (1, (uint8_t*)"Please Input Correct Argument!\n");
        return 2;       
    }
    running =  (arg_seq[0]-'0');
    if((running<0)||(running>9))
    {
        ece391_fdputs (1, (uint8_t*)"Please Input Seed from 0-9!\n");
        return 2;       
    }
    for(i=0;i<10;i++)
    {
        buflist[i] = rand_range(0,100);
    }
    guess_result = buflist[running];

    ece391_fdputs (1, (uint8_t*)"Please Input Your Guess from 0-100!\n");
    while(1)
    {
        length = ece391_read(0, arg_game, 1023);

        input_value = ece391_atoi(arg_game);


        if(input_value < guess_result){
            ece391_fdputs (1, (uint8_t*)"Lower, Please try again.\n");
            for(i=0;i<1024;i++)
            {
                arg_base[i] = (uint8_t) '\0';
                arg_seq[i] = (uint8_t)'\0';
                arg_game[i] = (uint8_t)'\0';
            }
            continue;
        }

        if(input_value > guess_result)
        {
            ece391_fdputs (1, (uint8_t*)"Higher, Please try again.\n");

            for(i=0;i<1024;i++)
            {
                arg_base[i] = (uint8_t) '\0';
                arg_seq[i] = (uint8_t)'\0';
                arg_game[i] = (uint8_t)'\0';
            }
            continue;
        }

        if(input_value == guess_result)
        {
            ece391_fdputs (1, (uint8_t*)"Congratulations! You win!\n");
            break;            
        }


    }

    return 0;

}


