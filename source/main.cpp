#include "mbed.h"
#include "stm32l475e_iot01_audio.h"

#define AUDIO_INSTANCE              BSP_AUDIO_IN_INSTANCE
#define AUDIO_VOLUME_VALUE          32
#define AUDIO_CHANNELS              1
#define PCM_AUDIO_IN_SAMPLES        (AUDIO_SAMPLING_FREQUENCY / 1000)

static uint16_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;

HAL_StatusTypeDef MX_DFSDM1_ClockConfig(DFSDM_Channel_HandleTypeDef *hDfsdmChannel, uint32_t SampleRate)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hDfsdmChannel);

  HAL_StatusTypeDef        status = HAL_OK;
  RCC_PeriphCLKInitTypeDef RCC_ExCLKInitStruct;

  /* Configure the SAI PLL according to the requested audio frequency */
  /* Retrieve actual RCC configuration */
  HAL_RCCEx_GetPeriphCLKConfig(&RCC_ExCLKInitStruct);

  /* Set the PLL configuration according to the audio frequency */
  /* SAI1 clock config
  PLLSAI1_VCO = (48 MHz / PLLSAI1M) * PLLSAI1N = 48 / 6 * 43 = 344
  SAI_CK_x = PLLSAI1_VCO/PLLSAI1P = 2344 / 7 = 49.142 MHz */
  RCC_ExCLKInitStruct.PeriphClockSelection    = RCC_PERIPHCLK_SAI1;
  RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1Source   = RCC_PLLSOURCE_MSI;
  RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1M        = 6;
  RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1N        = 43;
  RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1P        = 7;
  RCC_ExCLKInitStruct.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_SAI1CLK;
  RCC_ExCLKInitStruct.Sai1ClockSelection      = RCC_SAI1CLKSOURCE_PLLSAI1;
  status = HAL_RCCEx_PeriphCLKConfig(&RCC_ExCLKInitStruct);

  return status;
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance)
{
//   uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
//   uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

//   /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
//   memcpy(Fill_Buffer + index_buff_fill, PCM_Buffer, buffer_size);
//   index_buff_fill += nb_samples;

//   AudioProcess();

// #if SENSING1_USE_DATALOG
//   if (SD_LogAudio_Enabled)
//   {
//     AudioProcess_SD_Recording(PCM_Buffer, nb_samples);
//   }
//   else
// #endif /* SENSING1_USE_DATALOG */
//   {
//     if (W2ST_CHECK_CONNECTION(W2ST_CONNECT_AUDIO_LEVEL))
//     {
//       AudioProcess_DB_Noise();
//     }
//   }
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance)
{
//   uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
//   uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

//   /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
//   memcpy(Fill_Buffer + index_buff_fill, PCM_Buffer + nb_samples, buffer_size);
//   index_buff_fill += nb_samples;

//   AudioProcess();

// #if SENSING1_USE_DATALOG
//   if (SD_LogAudio_Enabled)
//   {
//     AudioProcess_SD_Recording(PCM_Buffer + nb_samples, nb_samples);
//   }
//   else
// #endif /* SENSING1_USE_DATALOG */
//   {
//     if (W2ST_CHECK_CONNECTION(W2ST_CONNECT_AUDIO_LEVEL))
//     {
//       AudioProcess_DB_Noise();
//     }
//   }
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance)
{
  printf("BSP_AUDIO_IN_Error_CallBack\n");
}

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
        printf("\nOK Audio Init\t(Audio Freq.= %ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Record (%d)\n", ret);
        return 0;
    }
    else {
        printf("OK Audio Record\n");
    }

    print_status_thread();
}
