#include "mbed.h"
#include "stm32l475e_iot01_audio.h"

#define AUDIO_INSTANCE              BSP_AUDIO_IN_INSTANCE
#define AUDIO_VOLUME_VALUE          32
#define AUDIO_CHANNELS              1
#define PCM_AUDIO_IN_SAMPLES        (AUDIO_SAMPLING_FREQUENCY / 1000)

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;

static DigitalOut led(LED1);
// static Thread status_thread;

void print_status_thread() {
    while (1) {
        led = !led;
        uint8_t *buf = (uint8_t*)PCM_Buffer;
        for (size_t ix = 0; ix < PCM_BUFFER_LEN; ix++) {
            printf("%02x", buf[ix]);
        }
        printf("\n");
        wait_ms(500);
    }
}

int main() {
    // status_thread.start(&print_status_thread);

    printf("Hello %f\n", MBED_BUILD_TIMESTAMP);

    int32_t ret;

    MicParams.BitsPerSample = 16;
    MicParams.ChannelsNbr = AUDIO_CHANNELS;
    MicParams.Device = AUDIO_IN_DIGITAL_MIC1;
    MicParams.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    MicParams.Volume = 32;

    ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &MicParams);

    if(ret != BSP_ERROR_NONE) {
        printf("\nError Audio Init (%d)\r\n", ret);
        return 1;
    } else {
        printf("\nOK Audio Init3\t(Audio Freq.= %ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Record (%d)\n", ret);
        return 0;
    }
    else {
        printf("OK Audio Record\n");
    }

    wait_ms(1);

    print_status_thread();
}
