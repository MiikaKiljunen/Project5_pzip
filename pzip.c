/*************************************************************************/
/* CT30A3370 Käyttöjärjestelmät ja systeemiohjelmointi
* Author: Miika Kiljunen
* Date: 31.7.2024
*/
/*************************************************************************/
/*Project 5: Parallel zip, pzip.c*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define CHUNK_SIZE 1024 //Allocate chunk size. This could be dynamic, but I already struggled way too much just to get this working.

//struct from the threads API textbook snippet
typedef struct {
    FILE *file;
    long start;
    long end;
    int thread_id;
} thread_data_t;

//declare the lock and condition variable
pthread_mutex_t lock;
pthread_cond_t cond;

//implement a chunk based processing system
void *process_chunk(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    FILE *file = data->file;
    long start = data->start;
    long end = data->end;

    fseek(file, start, SEEK_SET);

    char prev_char = '\0', char_from_stream;
    int incr = 0;

    //check if file is empty
    if (start > 0) {
        fseek(file, start - 1, SEEK_SET);//get the last character of the previous chunk and return to the start of the current chunk
        prev_char = fgetc(file);
        fseek(file, start, SEEK_SET);
    }

    //the logic for processing the files is still the same, now we implement a lock to write the data in parallel

    for (long i = start; i < end; i++) {
        char_from_stream = fgetc(file);
        if (char_from_stream == EOF) {
            break;
        }

        if (char_from_stream == prev_char) {
            incr++;
        }else {

            pthread_mutex_lock(&lock);
            fwrite(&incr, sizeof(int), 1, stdout);
            fwrite(&prev_char, sizeof(char), 1, stdout);
            pthread_mutex_unlock(&lock);

            prev_char = char_from_stream;

            incr = 1;
        }
    }


    //it would not work otherwise without this
    pthread_mutex_lock(&lock);
    fwrite(&incr, sizeof(int), 1, stdout);
    fwrite(&prev_char, sizeof(char), 1, stdout);
    pthread_mutex_unlock(&lock);

    return NULL;
}


int main(int argc, char *argv[]){

    if (argc < 2) {
        printf("pzip: file1 [file2 ...]\n");
        return 1;
    }

    //init lock and conditional variable
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // int incr = 0;
    // char prev_char = '\0';
    // int first_file = 1; //for transitioning to multiple files

    int num_threads = 4;  //threads amount, again, could be dynamic...
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    for(int i=1; i<argc; i++){

        FILE *unc_input_stream = NULL;

        unc_input_stream = fopen(argv[i], "r");

        if (unc_input_stream == NULL) {
            printf("pzip: cannot open file\n");
            return 1;
        }

        
        //determine file and chunk size
        fseek(unc_input_stream, 0L, SEEK_END);
        long file_size = ftell(unc_input_stream);
        fseek(unc_input_stream, 0L, SEEK_SET);

        long chunk_size = (file_size + num_threads - 1) / num_threads;

        //use struct to determine chunk
        for (int j = 0; j < num_threads; j++) {
            thread_data[j].file = unc_input_stream;
            thread_data[j].start = j * chunk_size;

            if(j == num_threads -1){
                thread_data[j].end = file_size;
            }
            else{
                thread_data[j].end = (j+1)*chunk_size;
            }

            thread_data[j].thread_id = j;
            pthread_create(&threads[j], NULL, process_chunk, &thread_data[j]);
        }

        //join threads after processing
        for (int k = 0; k < num_threads; k++) {
            pthread_join(threads[k], NULL);
        }

        fclose(unc_input_stream);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);

    return 0;
}

/* eof */