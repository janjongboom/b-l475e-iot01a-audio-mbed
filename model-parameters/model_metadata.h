#ifndef _EDGE_IMPULSE_MODEL_METADATA_H_
#define _EDGE_IMPULSE_MODEL_METADATA_H_

#define EDGE_INFERENCING_UTENSOR    1
#define EDGE_INFERENCING_TFLITE     2
#define INFERENCING_NN_INPUT_FRAME_SIZE         637
#define INFERENCING_RAW_SAMPLE_COUNT            16000
#define INFERENCING_RAW_SAMPLES_PER_FRAME       1
#define INFERENCING_DSP_INPUT_FRAME_SIZE        (INFERENCING_RAW_SAMPLE_COUNT * INFERENCING_RAW_SAMPLES_PER_FRAME)
#define INFERENCING_INTERVAL_MS                 0
#define INFERENCING_OUT_TENSOR_NAME             "y_pred/Softmax:0"
#define INFERENCING_LABEL_COUNT                 3
#define EDGE_IMPULSE_HAS_ANOMALY                0
// #define EDGE_IMPULSE_DSP_FN                     mfcc
#define EDGE_IMPULSE_TFLITE_ARENA_SIZE          (16 * 1024)
#define EDGE_INFERENCING                        EDGE_INFERENCING_TFLITE
#define EDGE_IMPULSE_HAS_SAMPLER                0

const char* inferencing_categories[] = { "no", "noise", "yes" };

#endif // _EDGE_IMPULSE_MODEL_METADATA_H_
