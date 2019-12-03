#include "mbed.h"
#include "stm32l475e_iot01_audio.h"

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;

static DigitalOut led(LED1);
static EventQueue ev_queue;

// Place to store final audio (alloc on the heap), here two seconds...
static size_t TARGET_AUDIO_BUFFER_NB_SAMPLES = AUDIO_SAMPLING_FREQUENCY * 2;
static int16_t *TARGET_AUDIO_BUFFER = (int16_t*)calloc(TARGET_AUDIO_BUFFER_NB_SAMPLES, sizeof(int16_t));
static size_t TARGET_AUDIO_BUFFER_IX = 0;

// we skip the first 50 events (100 ms.) to not record the button click
static size_t SKIP_FIRST_EVENTS = 50;
static size_t half_transfer_events = 0;
static size_t transfer_complete_events = 0;

// callback that gets invoked when TARGET_AUDIO_BUFFER is full
void target_audio_buffer_full() {
    // pause audio stream
    int32_t ret = BSP_AUDIO_IN_Pause(AUDIO_INSTANCE);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Pause (%d)\n", ret);
    }
    else {
        printf("OK Audio Pause\n");
    }

    // create WAV file
    size_t wavFreq = AUDIO_SAMPLING_FREQUENCY;
    size_t dataSize = (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);
    size_t fileSize = 44 + (TARGET_AUDIO_BUFFER_NB_SAMPLES * 2);

    uint8_t wav_header[44] = {
        0x52, 0x49, 0x46, 0x46, // RIFF
        fileSize & 0xff, (fileSize >> 8) & 0xff, (fileSize >> 16) & 0xff, (fileSize >> 24) & 0xff,
        0x57, 0x41, 0x56, 0x45, // WAVE
        0x66, 0x6d, 0x74, 0x20, // fmt
        0x10, 0x00, 0x00, 0x00, // length of format data
        0x01, 0x00, // type of format (1=PCM)
        0x01, 0x00, // number of channels
        wavFreq & 0xff, (wavFreq >> 8) & 0xff, (wavFreq >> 16) & 0xff, (wavFreq >> 24) & 0xff,
        0x00, 0x7d, 0x00, 0x00, // 	(Sample Rate * BitsPerSample * Channels) / 8
        0x02, 0x00, 0x10, 0x00,
        0x64, 0x61, 0x74, 0x61, // data
        dataSize & 0xff, (dataSize >> 8) & 0xff, (dataSize >> 16) & 0xff, (dataSize >> 24) & 0xff,
    };

    printf("Total complete events: %lu, index is %lu\n", transfer_complete_events, TARGET_AUDIO_BUFFER_IX);

    // print both the WAV header and the audio buffer in HEX format to serial
    // you can use the script in `hex-to-buffer.js` to make a proper WAV file again
    printf("WAV file:\n");
    for (size_t ix = 0; ix < 44; ix++) {
        printf("%02x", wav_header[ix]);
    }

    uint8_t *buf = (uint8_t*)TARGET_AUDIO_BUFFER;
    for (size_t ix = 0; ix < TARGET_AUDIO_BUFFER_NB_SAMPLES * 2; ix++) {
        printf("%02x", buf[ix]);
    }
    printf("\n");
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance) {
    half_transfer_events++;
    if (half_transfer_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2), PCM_Buffer, buffer_size);
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        ev_queue.call(&target_audio_buffer_full);
        return;
    }
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance) {
    transfer_complete_events++;
    if (transfer_complete_events < SKIP_FIRST_EVENTS) return;

    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    if ((TARGET_AUDIO_BUFFER_IX + nb_samples) > TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        return;
    }

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2),
        ((uint8_t*)PCM_Buffer) + (nb_samples * 2), buffer_size);
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES) {
        ev_queue.call(&target_audio_buffer_full);
        return;
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

void print_stats() {
    printf("Half %lu, Complete %lu, IX %lu\n", half_transfer_events, transfer_complete_events,
        TARGET_AUDIO_BUFFER_IX);
}

void start_recording() {
    int32_t ret;
    uint32_t state;

    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printf("Cannot start recording: Error getting audio state (%d)\n", ret);
        return;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printf("Cannot start recording: Already recording\n");
        return;
    }

    // reset audio buffer location
    TARGET_AUDIO_BUFFER_IX = 0;
    transfer_complete_events = 0;
    half_transfer_events = 0;

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Record (%ld)\n", ret);
        return;
    }
    else {
        printf("OK Audio Record\n");
    }
}

int main() {
    printf("Hello from the B-L475E-IOT01A microphone demo\n");

    if (!TARGET_AUDIO_BUFFER) {
        printf("Failed to allocate TARGET_AUDIO_BUFFER buffer\n");
        return 0;
    }

    // set up the microphone
    MicParams.BitsPerSample = 16;
    MicParams.ChannelsNbr = AUDIO_CHANNELS;
    MicParams.Device = AUDIO_IN_DIGITAL_MIC1;
    MicParams.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    MicParams.Volume = 32;

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &MicParams);

    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Init (%ld)\r\n", ret);
        return 1;
    } else {
        printf("OK Audio Init\t(Audio Freq=%ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

    printf("Press the BLUE button to record a message\n");

    // hit the blue button to record a message
    static InterruptIn btn(BUTTON1);
    btn.fall(ev_queue.event(&start_recording));

    ev_queue.dispatch_forever();
}
