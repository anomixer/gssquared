#include <cstdint>
#include <cstdio>
#include <iostream>

#include "clock.hpp"
#include "cpu.hpp"
#include "debug.hpp"
#include "devices/speaker/speaker.hpp"
#include "event_poll.hpp"

uint64_t debug_level;
bool write_output = false;

FILE *wav_file = NULL;

FILE *create_wav_file(const char *filename) {
    FILE *wav_file = fopen(filename, "wb");
    if (!wav_file) {
        std::cerr << "Error: Could not open wav file\n";
        return NULL;
    }

// write a WAV header:
    // RIFF header
    fwrite("RIFF", 1, 4, wav_file);
    
    // File size (to be filled later)
    uint32_t file_size = 0;
    fwrite(&file_size, 4, 1, wav_file);
    
    // WAVE header
    fwrite("WAVE", 1, 4, wav_file);
    
    // Format chunk
    fwrite("fmt ", 1, 4, wav_file);
    
    // Format chunk size (16 bytes)
    uint32_t fmt_chunk_size = 16;
    fwrite(&fmt_chunk_size, 4, 1, wav_file);
    
    // Audio format (1 = PCM)
    uint16_t audio_format = 1;
    fwrite(&audio_format, 2, 1, wav_file);
    
    // Number of channels (1 = mono)
    uint16_t num_channels = 1;
    fwrite(&num_channels, 2, 1, wav_file);
    
    // Sample rate (44100Hz)
    uint32_t sample_rate = SAMPLE_RATE;
    fwrite(&sample_rate, 4, 1, wav_file);
    
    // Byte rate = SampleRate * NumChannels * BitsPerSample/8
    uint32_t byte_rate = sample_rate * num_channels * 16/8;
    fwrite(&byte_rate, 4, 1, wav_file);
    
    // Block align = NumChannels * BitsPerSample/8
    uint16_t block_align = num_channels * 16/8;
    fwrite(&block_align, 2, 1, wav_file);
    
    // Bits per sample
    uint16_t bits_per_sample = 16;
    fwrite(&bits_per_sample, 2, 1, wav_file);
    
    // Data chunk header
    fwrite("data", 1, 4, wav_file);
    
    // Data chunk size (to be filled later)
    uint32_t data_size = 0;
    fwrite(&data_size, 4, 1, wav_file);

    return wav_file;
}

void finalize_wav_file(FILE *wav_file) {
    long file_size = ftell(wav_file);
    uint32_t riff_size = file_size - 8; // Exclude "RIFF" and its size field
    uint32_t data_size = file_size - 44; // Exclude header size (44 bytes)

    // Seek to RIFF size field and update
    fseek(wav_file, 4, SEEK_SET);
    fwrite(&riff_size, 4, 1, wav_file);

    // Seek to data size field and update
    fseek(wav_file, 40, SEEK_SET);
    fwrite(&data_size, 4, 1, wav_file);

    // Close the file
    fclose(wav_file);
}

void event_poll_local(cpu_state *cpu) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {

        }
    }
}

/* void *get_module_state(cpu_state *cpu, module_id_t module_id) {
    return cpu->module_store[module_id];
}

void set_module_state(cpu_state *cpu, module_id_t module_id, void *state) {
    cpu->module_store[module_id] = state;
} */

/* void register_C0xx_memory_read_handler(unsigned short address, unsigned char (*read_handler)(cpu_state*, unsigned short)) {
}

void register_C0xx_memory_write_handler(uint16_t address, memory_write_handler handler) {
} */

void usage(const char *exe) {
    std::cerr << "Usage: " << exe << " [-w] <recording file>\n";
    exit(1);
}

uint64_t audio_generate_frame_local(speaker_state_t *speaker_state /* computer_t *computer, */ /* cpu_state *cpu */  /* , uint64_t cycle_window_start, uint64_t cycle_window_end */) {

    //speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    speaker_config_t *sc = speaker_state->sp->config;
    static uint64_t last_hz_rate = 0;

    // the primary purpose of this is to put even more samples in - if we stretch a frame early on, the apple ii boot beep gets distorted.
    /* if (speaker_state->first_time) {
        speaker_state->first_time = false;
        printf("audio_generate_frame: first time send empty frame and a bit more\n");
        memset(speaker_state->working_buffer, 0, sc->samples_per_frame * sizeof(int16_t));
        SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, sc->samples_per_frame*sizeof(int16_t));
        //SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, sc->samples_per_frame*sizeof(int16_t));
        return sc->samples_per_frame*2;
    } */

    uint64_t queued_samples = SDL_GetAudioStreamQueued(speaker_state->stream) / sizeof(int16_t);
    int16_t addsamples = 0;
    if (queued_samples < (sc->samples_per_frame)) { // was a half frame, but we want more.
        addsamples = (sc->samples_per_frame - queued_samples) / 2; // half of what we need so we asymptotically approach the target.
        if (addsamples > 500) addsamples = 500;
        //printf("queue underrun %llu %f %f adding %d extra samples\n", queued_samples, speaker_state->amplitude, speaker_state->polarity, addsamples);
    }

    speaker_state->sp->generate_buffer_int(speaker_state->working_buffer, sc->samples_per_frame );

    if ( addsamples ) {
        int16_t extra_sample = speaker_state->working_buffer[sc->samples_per_frame - 1];
        for (int i = 0; i < addsamples; i++) {
            speaker_state->working_buffer[sc->samples_per_frame + i] = extra_sample;
        }
        SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, (sc->samples_per_frame + addsamples) * sizeof(int16_t));
    } else { 
        SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, sc->samples_per_frame * sizeof(int16_t));
     }
    
    return sc->samples_per_frame;
}

// Utility function to round up to the next power of 2
inline uint32_t next_power_of_2(uint32_t value) {
    if (value == 0) return 1;
    if ((value & (value - 1)) == 0) return value; // Already a power of 2
    
    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    
    return value;
}

void init_mb_speaker_local(speaker_state_t *speaker_state, clock_mode_info_t *clock) {

    //speaker_state_t *speaker_state = new speaker_state_t;

    double frame_rate = (double)clock->c14M_per_second / (double)clock->c14M_per_frame;
    double cpu_rate = (double)clock->eff_cpu_rate;

    //speaker_config_t *sc = new speaker_config_t(/* 59.9227f */ frame_rate, /* 1020484.0f */ cpu_rate, 44343.0f, 44343.0f / frame_rate /* 740 */);
    speaker_config_t *sc = new speaker_config_t(frame_rate, cpu_rate, 44100.0f, 44100.0f / frame_rate );
    EventBufferRing *eb = new EventBufferRing(next_power_of_2(120'000));
    speaker_state->sp = new Speaker(sc, eb);
    speaker_state->event_buffer = eb;
    // make sure we allocate plenty of room for extra samples for catchup in generate.
    uint16_t bufsize = next_power_of_2(sc->samples_per_frame * 2);
    speaker_state->working_buffer = new int16_t[bufsize];
    //speaker_state->computer = computer;
    
    //speaker_state->cpu = cpu;
    speaker_state->speaker_recording = nullptr;

    //set_module_state(cpu, MODULE_SPEAKER, speaker_state);

	// Initialize SDL audio - is this right, to do this again here?
	SDL_Init(SDL_INIT_AUDIO);
	
    SDL_AudioSpec desired = {};
    //desired.freq = SAMPLE_RATE;
    desired.freq = sc->output_rate;
    desired.format = SDL_AUDIO_S16LE;
    desired.channels = 1;

    speaker_state->device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (speaker_state->device_id == 0) {
        SDL_Log("Couldn't open audio device: %s", SDL_GetError());
        return;
    }

    speaker_state->stream = SDL_CreateAudioStream(&desired, NULL);
    if (!speaker_state->stream) {
        SDL_Log("Couldn't create audio stream: %s", SDL_GetError());
        return;
    } else if (!SDL_BindAudioStream(speaker_state->device_id, speaker_state->stream)) {  /* once bound, it'll start playing when there is data available! */
        SDL_Log("Failed to bind speaker stream to device: %s", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(speaker_state->device_id);

    // prime the pump with a few frames of silence - make sure we use the correct frame size so we pass the check in generate.
    /* memset(speaker_state->working_buffer, 0, speaker_state->sp->config->samples_per_frame * sizeof(int16_t));
    SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, speaker_state->sp->config->samples_per_frame*sizeof(int16_t)); */

    speaker_state->preFilter = new LowPassFilter();
    speaker_state->preFilter->setCoefficients(8000.0f, (double)1020500); // 1020500 is actual possible sample rate of input toggles.
    speaker_state->postFilter = new LowPassFilter();
    speaker_state->postFilter->setCoefficients(8000.0f, (double)SAMPLE_RATE);

}

void speaker_start(speaker_state_t *speaker_state) {
    //speaker_state_t *speaker_state = (speaker_state_t *)get_module_state(cpu, MODULE_SPEAKER);
    speaker_config_t *sc = speaker_state->sp->config;
    
    int x = SDL_GetAudioStreamQueued(speaker_state->stream);

    // Start audio playback
    // put a frame of blank audio into the buffer to prime the pump.
    memset(speaker_state->working_buffer, 0, sc->samples_per_frame * sizeof(int16_t));
    bool ret = SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, sc->samples_per_frame*sizeof(int16_t));
    ret = SDL_PutAudioStreamData(speaker_state->stream, speaker_state->working_buffer, sc->samples_per_frame*sizeof(int16_t));

    x = SDL_GetAudioStreamQueued(speaker_state->stream);

    if (!SDL_ResumeAudioDevice(speaker_state->device_id)) {
        std::cerr << "Error resuming audio device: " << SDL_GetError() << std::endl;
    }

    speaker_state->device_started = 1;
}

int main(int argc, char **argv) {
    debug_level = DEBUG_SPEAKER;

    cpu_state *cpu = new cpu_state(PROCESSOR_65C02);
    speaker_state_t *speaker_state = new speaker_state_t();

    select_system_clock(CLOCK_SET_PAL);

    MMU_II *mmu = new MMU_II(256, 48*1024, nullptr);
    cpu->set_mmu(mmu);

    // Parse command line arguments
    if (argc < 2) {
        usage(argv[0]);
    }

    int recording_file_index = 1;
    if (strcmp(argv[1], "-w") == 0) {
        write_output = true;
        recording_file_index = 2;
        if (argc < 3) {
            usage(argv[0]);
        }
    }

    // load 'recording' file into the log
    FILE *recording = fopen(argv[recording_file_index], "r");
    if (!recording) {
        std::cerr << "Error: Could not open recording file\n";
        return 1;
    }
    clock_mode_info_t *clock = &system_clock_mode_info[CLOCK_1_024MHZ];

    init_mb_speaker_local(speaker_state, clock);
    printf("clock mode         : %llu\n", clock->hz_rate);
    printf("c14M_per_second    : %llu\n", clock->c14M_per_second);
    printf("c14M_per_frame     : %llu\n", clock->c14M_per_frame);
    printf("c_14M_per_cpu_cycle: %llu\n", clock->c_14M_per_cpu_cycle);
    printf("extra_per_scanline : %llu\n", clock->extra_per_scanline);
    printf("cycles_per_scanline: %llu\n", clock->cycles_per_scanline);
    printf("us_per_frame_even  : %llu\n", clock->us_per_frame_even);
    printf("us_per_frame_odd   : %llu\n", clock->us_per_frame_odd);
    printf("eff_cpu_rate       : %llu\n", clock->eff_cpu_rate);
    // load events into the event buffer that is allocated by the speaker module.
    uint64_t event;

    // skip any long silence, start playback / reconstruction at first event.
    uint64_t first_event = 0;
    uint64_t last_event = 0;

    while (fscanf(recording, "%llu", &event) != EOF) {
        speaker_state->event_buffer->add_event(event);
        if (first_event == 0) {
            first_event = event;
        }
        last_event = event;
    }
    fclose(recording);

    speaker_start(speaker_state);

    cpu->cycles = first_event;

    if (write_output) {
        wav_file = create_wav_file("test.wav");
    }
    uint64_t cycle_window_last = 0;
    uint64_t num_frames = ((last_event - first_event) / 17030);

    printf("first_event: %llu last_event: %llu\n", first_event, last_event);
    printf("num_frames: %llu\n", num_frames);

    for (int i = 0; i < num_frames; i++) {
        event_poll_local(cpu);
        
        uint64_t samps = audio_generate_frame_local(speaker_state);
        if (write_output) {
            fwrite(speaker_state->working_buffer, sizeof(int16_t), samps, wav_file);
        }
        cycle_window_last = cpu->cycles;
        cpu->cycles += 17030;
        
        SDL_Delay(5);
        printf("frame %d\r", i);
        fflush(stdout);

    }
    printf("\n");
    int queued = 0;
    if (!write_output) {
        while ((queued = SDL_GetAudioStreamAvailable(speaker_state->stream)) > 0) {
            //printf("queued: %d\n", queued);
            event_poll_local(cpu);
            printf("queued: %d\r", queued);
            fflush(stdout);    
        }
    }
    if (write_output) {
        finalize_wav_file(wav_file);
    }

    return 0;
}