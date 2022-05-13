/*
Comp Eng 3DY4 (Computer Systems Integration Project)
Copyright by Nicola Nicolici
Department of Electrical and Computer Engineering
McMaster University
Ontario, Canada
*/

#include "dy4.h"
#include "filter.h"
#include "fourier.h"
#include "genfunc.h"
#include "iofunc.h"
#include "logfunc.h"
#include "sampling.h"
#include "rf_fe_block.h"
#include "mono_block.h"
#include "stereo_block.h"
#include "rds_block.h"
#include "channel.h"
#include "performance.h"

#include <semaphore.h>
#include <unistd.h>
#include <queue>
#include <thread>
#include <string>

sem_t audio_mutex;
sem_t rds_mutex;
sem_t mono_mutex;
sem_t mono_done;


void mono_processing_thread(MonoBlock &mono_block,
                            std::vector<double> &fm_demod_data_block_out,
                            std::vector<double> &mono_audio_data_block){

    while (1) {
        // block execution until mono done
        sem_wait(&mono_mutex);

	    // do mono path
        mono_block.process(fm_demod_data_block_out, mono_audio_data_block);
        // Release control
        sem_post(&mono_done);
    }
}

void audio_consumer_thread(std::queue<std::vector<double>> &audio_queue,
                           unsigned int block_size,
                           MonoBlock &mono_block, StereoBlock &stereo_block,
                           std::vector<double> &fm_demod_data_block_out,
                           std::vector<double> &mono_audio_data_block,
                           std::vector<double> &stereo_audio_data_block,
                           std::vector<double> &left_channel_audio_data_block,
                           std::vector<double> &right_channel_audio_data_block) {

    unsigned int block_count = 0;
    // Make a deep copy

    // Setup the intermediate vectors for usage
    mono_block.setup_vectors(fm_demod_data_block_out.size());
    stereo_block.setup_vectors(fm_demod_data_block_out.size());

    // Sleep a little before the start to make sure enough samples in the queue
    sleep(0.1);


    // FIXME: disabled for now
    while (1){

        // If we're running low on samples, wait very slightly
        while(audio_queue.size() <= 0) {
            sleep(0.0001);
        }
        sem_wait(&audio_mutex);

        fm_demod_data_block_out = audio_queue.front();

        audio_queue.pop();

        sem_post(&audio_mutex);

        // FIXME: for performance measuring, delete when not using
        //Performance timer;

        //timer.begin();

        //activate mono thread
        sem_post(&mono_mutex);

        // do stereo path
        stereo_block.process(fm_demod_data_block_out, stereo_audio_data_block);

        // Wait for mono to be done
        sem_wait(&mono_done);

        add_channels(mono_audio_data_block, stereo_audio_data_block,
                     left_channel_audio_data_block);
        sub_channels(mono_audio_data_block, stereo_audio_data_block,
                     right_channel_audio_data_block);


        writeStereoStdoutBlockData(left_channel_audio_data_block, right_channel_audio_data_block);
        //writeMonoStdoutBlockData(mono_audio_data_block);
        //timer.stop();
        block_count += 1;
    }
}

void rds_consumer_thread(bool disable,
                         std::queue<std::vector<double>> &rds_queue,
                         unsigned int block_size,
                         RdsBlock &rds_block,
                         std::vector<double> &fm_demod_data_block_ref,
                         std::vector<double> &test_vec) {
    unsigned int block_count = 0;

    //do nothing if disabled
    if (disable) {
        return;
    }

    // Make a deep copy
    std::vector<double> fm_demod_data_block(fm_demod_data_block_ref);

    // setup the intermediate vectors for usage
    rds_block.setup_vectors(fm_demod_data_block.size());

    // sleep a little before the start to maek sure enough samples in the queue
    sleep(0.1);

    while (!disable){
        // If we're running low on samples, wait very slightly
        //std::cerr << "Processing block " << block_count << "\n";
        while(rds_queue.size() <= 0) {
            sleep(0.0001);
        }
        sem_wait(&rds_mutex);

        fm_demod_data_block = rds_queue.front();

        rds_queue.pop();

        sem_post(&rds_mutex);

        rds_block.process(fm_demod_data_block, test_vec);
        //if (block_count == 9) std::exit(1);
        block_count += 1;
    }
}

int main(int argc, char *argv[])
{
    // Read input
    int operation_mode;
    if (argc == 1){
        operation_mode = 0;
    } else if (argc == 2){
        std::cerr << "Using operating mode " << argv[1] << "\n";
        operation_mode = std::stoi(argv[1]);
    } else {
        std::cerr << "Expected to have at most one argument.\n";
        return -1;
    }

    // create block objects
    RfFeBlock rf_fe_block (operation_mode);
    MonoBlock mono_block (operation_mode);
    StereoBlock stereo_block (operation_mode);
    RdsBlock rds_block (operation_mode);

    bool disable_rds;
    if (operation_mode == 0 || operation_mode == 2)
        disable_rds = false;
    else
        disable_rds = true;

    unsigned int block_size;

    if (!disable_rds){
        int lcm_decim = compute_lcm(mono_block.down_decim, rds_block.down_decim);
        if (lcm_decim < 50) {
            block_size = 1024*2*lcm_decim*rf_fe_block.rf_decim*2;
        } else {
            block_size = 2*lcm_decim*rf_fe_block.rf_decim*2;
        }
    } else {
        if (mono_block.down_decim < 50) {
            block_size = 1024*2*mono_block.down_decim*rf_fe_block.rf_decim*2;
        } else {
            block_size = 2*mono_block.down_decim*rf_fe_block.rf_decim*2;
        }
    }
    std::cerr << block_size << "\n";

    // Initialize intermediate blocks
    std::vector<float> iq_data_block;
    iq_data_block.assign(block_size, 0.0);
    std::vector<double> fm_demod_data_block_in;
    fm_demod_data_block_in.assign(iq_data_block.size()/rf_fe_block.rf_decim/2, 0.0);
    std::vector<double> fm_demod_data_block_audio;
    fm_demod_data_block_audio.assign(iq_data_block.size()/rf_fe_block.rf_decim/2, 0.0);
    std::vector<double> fm_demod_data_block_rds;
    fm_demod_data_block_rds.assign(iq_data_block.size()/rf_fe_block.rf_decim/2, 0.0);
    std::vector<double> mono_audio_data_block;
    mono_audio_data_block.assign(fm_demod_data_block_in.size()*mono_block.up_decim/mono_block.down_decim, 0.0);
    std::vector<double> stereo_audio_data_block;
    stereo_audio_data_block.assign(fm_demod_data_block_in.size()*stereo_block.up_decim/stereo_block.down_decim, 0.0);
    std::vector<double> left_channel_audio_data_block;
    left_channel_audio_data_block.assign(fm_demod_data_block_in.size()*stereo_block.up_decim/stereo_block.down_decim, 0.0);
    std::vector<double> right_channel_audio_data_block;
    right_channel_audio_data_block.assign(fm_demod_data_block_in.size()*stereo_block.up_decim/stereo_block.down_decim, 0.0);

    std::vector<double> test_vec;


    // Create queues for storing iq_data
    std::queue<std::vector<double>> audio_queue;
    std::queue<std::vector<double>> rds_queue;
    
    // Start mono and stereo consumer thread
    std::thread audio_consumer =
            std::thread(audio_consumer_thread,
                        std::ref(audio_queue),
                        std::ref(block_size), std::ref(mono_block), std::ref(stereo_block),
                        std::ref(fm_demod_data_block_audio), std::ref(mono_audio_data_block),
                        std::ref(stereo_audio_data_block),
                        std::ref(left_channel_audio_data_block),
                        std::ref(right_channel_audio_data_block));
 
    std::thread rds_consumer =
            std::thread(rds_consumer_thread,
                        std::ref(disable_rds), std::ref(rds_queue),
                        std::ref(block_size), std::ref(rds_block),
                        std::ref(fm_demod_data_block_rds), std::ref(test_vec));

    std::thread mono_processing =
            std::thread(mono_processing_thread,
                        std::ref(mono_block),
                        std::ref(fm_demod_data_block_audio), std::ref(mono_audio_data_block));


    sem_init(&audio_mutex, 0, 1);
    sem_init(&rds_mutex, 0, 1);
    sem_init(&mono_mutex, 0, 0);
    sem_init(&mono_done, 0, 0);

    //RF FE thread
    rf_fe_block.setup_vectors(iq_data_block.size());

    while (1){
        // read iq data
        readStdinBlockData(block_size, iq_data_block);
        // error checking for end of stream
        if ((std::cin.rdstate()) != 0) {
            std::cerr << "End of input stream reached" << std::endl;
            std::exit(1);
        }

        // TODO: Move to producer thread
	    // do RF front-end processing
	    rf_fe_block.process(iq_data_block, fm_demod_data_block_in);

        // TODO: push iq_data to queue
        // wait until both queues have space
        while(audio_queue.size() >= MAX_QUEUE_SIZE || rds_queue.size() >= MAX_QUEUE_SIZE) {
            sleep(0.0001);
        }
        sem_wait(&audio_mutex);
        sem_wait(&rds_mutex);

        audio_queue.push(fm_demod_data_block_in);
        if (!disable_rds) rds_queue.push(fm_demod_data_block_in);
        
        sem_post(&audio_mutex);
        sem_post(&rds_mutex);
    }

    audio_consumer.join();
    rds_consumer.join();
    mono_processing.join();
    
	return 0;
}
