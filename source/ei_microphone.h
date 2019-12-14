#ifndef _EI_MICROPHONE_H_
#define _EI_MICROPHONE_H_

#include "stm32l475e_iot01_audio.h"

/**
 * todo: we're using raw block device here because it's much faster
 *       however, we don't do wear-leveling at this point because of this
 *       we need to implement this.
 */

#define BD_ERASE_SIZE                   4096

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t mic_params;

// buffer, needs to be aligned to temp flash erase size
#define AUDIO_BUFFER_NB_SAMPLES         512

// we'll use only half the buffer at the same time, because we have flash latency
// of up to 70 ms.
static EIDSP_i16 AUDIO_BUFFER[AUDIO_BUFFER_NB_SAMPLES];
static size_t AUDIO_BUFFER_IX = 0;

#define AUDIO_THREAD_STACK_SIZE             3072
static uint8_t AUDIO_THREAD_STACK[AUDIO_THREAD_STACK_SIZE];

// skip the first 2000 ms.
static bool is_recording = false;
static uint32_t audio_event_count = 0;
static uint32_t max_audio_event_count = 0;

extern BlockDevice *bd;
static EventQueue mic_queue;
static bool is_uploaded = false;
static size_t TEMP_FILE_IX = 0;

extern DigitalOut led;

enum AUDIO_BUFFER_EVENT {
    HALF = 0,
    FULL = 1,
    FINALIZE = 2,
    NONE = 3
};

AUDIO_BUFFER_EVENT last_audio_buffer_event = NONE;

void audio_buffer_callback(AUDIO_BUFFER_EVENT event) {
    Timer t;
    t.start();
    int16_t *start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * event);
    size_t buffer_size = (AUDIO_BUFFER_NB_SAMPLES / 2) * sizeof(int16_t);

    if (event == FINALIZE) {
        if (last_audio_buffer_event == HALF) {
            start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * FULL);
            buffer_size = (AUDIO_BUFFER_IX - (AUDIO_BUFFER_NB_SAMPLES / 2)) * sizeof(int16_t);
        }
        else {
            start = AUDIO_BUFFER + ((AUDIO_BUFFER_NB_SAMPLES / 2) * HALF);
            buffer_size = AUDIO_BUFFER_IX * sizeof(int16_t);
        }
    }

    int ret = 0;
    if (TEMP_FILE_IX % BD_ERASE_SIZE == 0) {
        // printf("erase addr=%u, size=%u\n", TEMP_FILE_IX, buffer_size);
        ret = bd->erase(TEMP_FILE_IX, BD_ERASE_SIZE);
    }
    if (ret == 0) {
        ret = bd->program(start, TEMP_FILE_IX, buffer_size);
    }
    TEMP_FILE_IX += buffer_size;
    // int ret = fwrite((const void*)start, 2, buffer_size, temp_file);
    t.stop();

    last_audio_buffer_event = event;
}

void finish_sampling() {
    // printf("AUDIO_BUFFER_IX is %u\n", AUDIO_BUFFER_IX);

    led = 0;

    // end it
    if (TEMP_FILE_IX > 1000 * (AUDIO_SAMPLING_FREQUENCY / 1000) * 2) {
        TEMP_FILE_IX = 1000 * (AUDIO_SAMPLING_FREQUENCY / 1000) * 2;
    }
    printf("Done recording audio, total bytes collected: %u\n", TEMP_FILE_IX);

    is_uploaded = true;
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance) {
    if (!is_recording) return;
    if (audio_event_count >= max_audio_event_count) return;
    audio_event_count++;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((AUDIO_BUFFER_IX + nb_samples) > AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(AUDIO_BUFFER + AUDIO_BUFFER_IX, PCM_Buffer, buffer_size);
    AUDIO_BUFFER_IX += nb_samples;

    if (AUDIO_BUFFER_IX == (AUDIO_BUFFER_NB_SAMPLES / 2)) {
        // half full
        mic_queue.call(&audio_buffer_callback, HALF);
    }
    else if (AUDIO_BUFFER_IX == AUDIO_BUFFER_NB_SAMPLES) {
        // completely full
        AUDIO_BUFFER_IX = 0;
        mic_queue.call(&audio_buffer_callback, FULL);
    }
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance) {
    if (!is_recording) return;
    if (audio_event_count >= max_audio_event_count) return;
    audio_event_count++;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((AUDIO_BUFFER_IX + nb_samples) > AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(AUDIO_BUFFER + AUDIO_BUFFER_IX, PCM_Buffer + nb_samples, buffer_size);
    AUDIO_BUFFER_IX += nb_samples;

    if (AUDIO_BUFFER_IX == (AUDIO_BUFFER_NB_SAMPLES / 2)) {
        // half full
        mic_queue.call(&audio_buffer_callback, HALF);
    }
    else if (AUDIO_BUFFER_IX == AUDIO_BUFFER_NB_SAMPLES) {
        // completely full
        AUDIO_BUFFER_IX = 0;
        mic_queue.call(&audio_buffer_callback, FULL);
    }
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance) {
    printf("BSP_AUDIO_IN_Error_CallBack\n");
}

bool ei_microphone_init() {
    mic_params.BitsPerSample = 16;
    mic_params.ChannelsNbr = AUDIO_CHANNELS;
    mic_params.Device = AUDIO_IN_DIGITAL_MIC1;
    mic_params.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    mic_params.Volume = 32;

    // if (!AUDIO_BUFFER) {
    //     printf("Failed to allocate audio buffer\n");
    //     return false;
    // }

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &mic_params);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Init (%ld)\r\n", ret);
        return false;
    }
    return true;
}

/**
 * Sample raw data
 */
bool ei_microphone_sample_start() {
    Thread queue_thread(osPriorityHigh, AUDIO_THREAD_STACK_SIZE, AUDIO_THREAD_STACK, "audio-thread-stack");
    queue_thread.start(callback(&mic_queue, &EventQueue::dispatch_forever));

    int32_t ret;
    uint32_t state;
    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printf("Cannot start recording: Error getting audio state (%ld)\n", ret);
        return false;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printf("Cannot start recording: Already recording\n");
        return false;
    }

    AUDIO_BUFFER_IX = 0;
    is_recording = false;
    is_uploaded = false;
    audio_event_count = 0;
    max_audio_event_count = 1000; // each event is 1ms., very convenient
    TEMP_FILE_IX = 0;

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error failed to start recording (%ld)\n", ret);
        return false;
    }

    printf("Starting in %d seconds...\n", 2000);

    ThisThread::sleep_for(2000);

    printf("Starting...\n");

    led = 1;

    is_recording = true;

    ThisThread::sleep_for(1000);

    // we're not perfectly aligned  with the audio interrupts, so give some extra time to ensure
    // we always have at least the sample_length_ms number of frames
    int32_t extra_time_left = 1000;
    while (audio_event_count < max_audio_event_count) {
        ThisThread::sleep_for(10);
        extra_time_left -= 10;
        if (extra_time_left <= 0) {
            break;
        }
    }
    is_recording = false;

    ret = BSP_AUDIO_IN_Stop(AUDIO_INSTANCE);
    if (ret != BSP_ERROR_NONE) {
        printf("Error failed to stop recording (%ld)\n", ret);
        return false;
    }

    audio_buffer_callback(FINALIZE);

    finish_sampling();

    return true;
}

#endif // _EI_MICROPHONE_H_
