/* IOS - Project 2.
 * Author: Martin Zovinec (xzovin00)
 */

#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>

#define MMAP(ptr) {(ptr) = mmap(NULL, sizeof((ptr)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(ptr) {munmap((ptr), sizeof((ptr)));}

// Function declaration
void errprint(char string[], int err_code);
int charToInt(char arg[], int num);
void cleanAll();
void memInit();
int semInit();

// Shared memory
int *instruction_count = NULL;      // A
int *entered_count = NULL;          // NE
int *registered_count = NULL;       // NC
int *in_building_count = NULL;      // NB
int *is_judge_present = NULL;

// Shared output file
FILE *output_file;

// Semaphores
sem_t *no_judge = NULL;
sem_t *mutex = NULL;
sem_t *confirmed = NULL;
sem_t *all_signed_in = NULL;

// Error print function for better readability
void errprint(char string[], int err_code) {
    fprintf(stderr,"Error %d: %s \n", err_code, string);
    exit(err_code);
}

// A function for char to int conversion like atoi, but it is much safer
int charToInt(char arg[], int num){
    char *chars;
    num = strtol(arg, &chars, 10);
    if(strlen(chars)>0)
        errprint("Nektery argument obsahuje nepovolene znaky!",1);
    return num;
}

//----------------------------------// Functions for initialization //----------------------------------//

int semInit(){ 
    if ((no_judge = sem_open("/xzovin00.ios.no_judge", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        fprintf(stderr, "Error: Opening no_judge semaphore failed.\n");
        return -1;
    }
    if ((mutex = sem_open("/xzovin00.ios.mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        fprintf(stderr, "Error: Opening mutex semaphore failed.\n");
        return -1;
    }
    if ((confirmed = sem_open("/xzovin00.ios.confirmed", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
        fprintf(stderr, "Error: Opening confirmed semaphore failed.\n");
        return -1;
    }
    if ((all_signed_in = sem_open("/xzovin00.ios.all_signed_in", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
        fprintf(stderr, "Error: Opening all_signed_in semaphore failed.\n");
        return -1;
    }
    return 0;
}


void memInit(){
    MMAP(instruction_count);
    MMAP(entered_count);
    MMAP(registered_count);
    MMAP(in_building_count);
    MMAP(is_judge_present);

    *instruction_count = 0;
    *entered_count = 0;
    *registered_count = 0;
    *in_building_count = 0;
    *is_judge_present = 0;
}

//------------------------------------------------------------------------------------------------//

void cleanAll(){
    UNMAP(instruction_count);
    UNMAP(entered_count);
    UNMAP(registered_count);
    UNMAP(in_building_count);
    UNMAP(is_judge_present);

    sem_close(no_judge);
    sem_close(mutex);
    sem_close(confirmed);
    sem_close(all_signed_in);

    sem_unlink("xzovin00.ios.no_judge");
    sem_unlink("xzovin00.ios.mutex");    
    sem_unlink("xzovin00.ios.confirmed");
    sem_unlink("xzovin00.ios.all_signed_in");

    fclose(output_file);
}


void immigrantGenerator ( int PI, int IG, int IT){
    // Immigrant generator
    for(int i=1;i<=PI;i++){

        // New immigrant
        usleep((rand() % (IG+1)) * 1000);
        pid_t imigrant = fork();
        if( imigrant == 0 ){
            // New immigrant starts
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: starts \n", ++(*instruction_count), i);


            // New immigrant enters if judge is not in the building
            sem_wait(no_judge);
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: enters\t\t: %d    : %d    : %d\n", ++(*instruction_count),i, ++(*entered_count), *registered_count, ++(*in_building_count));
            sem_post(no_judge);


            // Immigrant checks in if there is noone waiting in front of him
            sem_wait(mutex);
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: checks\t\t: %d    : %d    : %d\n", ++(*instruction_count),i, *entered_count, ++(*registered_count), *in_building_count);


            // If he is last one in the building and the judge is present, confirmation can begin
            
            if(*entered_count == *registered_count && *is_judge_present == 1){
                sem_post(all_signed_in);
            }else{
                sem_post(mutex);                   
            }


            // Immigrant wants certificate if the confirmation has ended
            sem_wait(confirmed);
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: wants certificate\t: %d    : %d    : %d\n", ++(*instruction_count),i, *entered_count, *registered_count, *in_building_count);


            // After some time he gets certificate
            usleep((rand() % (IT+1)) * 1000);
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: got certificate\t: %d    : %d    : %d\n", ++(*instruction_count),i, *entered_count, *registered_count, *in_building_count);


            // Leaves the building if the judge had already left
            sem_wait(no_judge);
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: IMM %d\t\t: leaves\t\t: %d    : %d    : %d\n", ++(*instruction_count),i, *entered_count, *registered_count, --(*in_building_count));
            sem_post(no_judge);
            exit(0);
        }
    }


    // Wait(NULL) for each process
    for(int i=1;i<=PI;i++){
        wait(NULL);
    }
    exit(0);

}

void judge (int remaining, int JT, int JG){
    int can_pass = 0;
    while(remaining > 0){

        // After some time judge wants to enter
        usleep((rand() % (JG+1)) * 1000);
        setbuf(output_file, NULL);
        sem_wait(no_judge);
        sem_wait(mutex);
        fprintf(output_file,"%d\t\t: JUDGE\t\t: wants to enter \n", ++(*instruction_count));

        // Judge enters after next immigrant enters
        
        setbuf(output_file, NULL);
        fprintf(output_file,"%d\t\t: JUDGE\t\t: enters\t\t: %d    : %d    : %d\n", ++(*instruction_count), *entered_count, *registered_count, *in_building_count);
        *is_judge_present = 1;


        // If not all immigrants have checked in, judge waits until they do so
        if (*entered_count != *registered_count){
            setbuf(output_file, NULL);
            fprintf(output_file,"%d\t\t: JUDGE\t\t: waits for IMM\t\t: %d    : %d    : %d\n", ++(*instruction_count), *entered_count, *registered_count, *in_building_count);
            sem_post(mutex);
            sem_wait(all_signed_in);
        }

        // After all have signed in, judge procedes to start the confirmation
        setbuf(output_file, NULL);
        fprintf(output_file,"%d\t\t: JUDGE\t\t: starts confirmation\t: %d    : %d    : %d\n", ++(*instruction_count), *entered_count, *registered_count, *in_building_count);
        

        // After some time confirmation ends and number of registered and the number of entered are set to zero
        usleep((rand() % (JT+1)) * 1000);
        remaining = remaining - *entered_count;
        can_pass = *entered_count;
        *entered_count = 0;
        *registered_count = 0;

        setbuf(output_file, NULL);
        fprintf(output_file,"%d\t\t: JUDGE\t\t: ends confirmation\t: %d    : %d    : %d\n", ++(*instruction_count), *entered_count, *registered_count, *in_building_count);
        // Confirmed semaphore is incremented by the amount of confirmed immigrants
        for(int a = 0; a < can_pass; a++){
            sem_post(confirmed);
        }


        // After some time judge leaves
        usleep((rand() % (JT+1)) * 1000);
        setbuf(output_file, NULL);
        fprintf(output_file,"%d\t\t: JUDGE\t\t: leaves\t\t: %d    : %d    : %d\n", ++(*instruction_count), *entered_count, *registered_count, *in_building_count);
        *is_judge_present = 0;
        sem_post(mutex);
        sem_post(no_judge);
    }

    setbuf(output_file, NULL);
    fprintf(output_file,"%d\t\t: JUDGE\t\t: finishes\n", ++(*instruction_count));
    exit(0);
}

int main (int argc, char *argv[]) {
    int PI,IG,JG,IT,JT;   
    PI = IG = JG = IT = JT = 0;

    // Set output file
    output_file = fopen("proj2.out", "w");

    // Setup for random
    srand(time(0));

    //------------// Argument check //------------//

    if (argc != 6 )
        errprint("Spatny pocet argumentu!",1);

    if ((PI = charToInt(argv[1],PI)) < 1){
        errprint("PI, the first argument, must be grester thsn one!",1);
        // A variable to tell judge when he is supposed to finish
        int remaining = PI;     
    }
    if ((IG = charToInt(argv[2],IG)) < 0 || IG > 2000)
        errprint("IG, the second argument, must be greater than zero and smaller than 2000!",1);

    if ((JG = charToInt(argv[3],JG)) < 0 || JG > 2000)
        errprint("JG, the third argument, must be greater than zero and smaller than 2000!",1);

    if ((IT = charToInt(argv[4],IT)) < 0 || IT > 2000)
        errprint("IT, the fourth argument, must be greater than zero and smaller than 2000!",1);

    if ((JT = charToInt(argv[5],JT)) < 0 || JT > 2000)
        errprint("JT, the fifth argument, must be greater than zero and smaller than 2000!",1);

    //--------------------------------------------//

    // Inicialization
    memInit();

    // If semaphore initialization fails a cleanup is imminent
    if (semInit() == -1){
        cleanAll();
        exit(1);
    }

    // Fork for spliting the program into a immigrant generator and judge
    pid_t fork1 = fork();
    if( fork1 == 0 ){
        immigrantGenerator( PI, IG, IT);
    }else{
        pid_t fork2 = fork();

            if( fork2 == 0 ){
                judge(remaining, JT, JG);
            }
    }

    // Wait(NULL) for each process
    wait(NULL);
    wait(NULL);
    cleanAll();
    return 0;
}
