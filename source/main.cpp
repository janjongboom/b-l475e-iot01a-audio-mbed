#include "mbed.h"
#include "stm32l475e_iot01_audio.h"
#include "speechpy.hpp"
#include "ei_microphone.h"
#include "model_metadata.h"
#include "ei_run_classifier.h"

using namespace ei;

DigitalOut led(LED1);
static EventQueue ev_queue;

static signal_t raw_audio_signal;
static signal_t preemphasized_audio_signal;

static void print_matrix(const char *name, matrix_t *matrix);

BlockDevice *bd = BlockDevice::get_default_instance();

/**
 * Get raw audio signal data
 */
static int raw_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    EIDSP_i16 *temp_buffer = (EIDSP_i16*)malloc(length * sizeof(EIDSP_i16));
    bd->read(temp_buffer, offset, length * sizeof(EIDSP_i16));
    int r = numpy::int16_to_float(temp_buffer, out_ptr, length);
    free(temp_buffer);
    return r;
}

/**
 * Preemphasize the audio signal (lazily)
 * In the constructor we tell that the source for the preemphasis is the raw audio signal.
 */
static class speechpy::processing::preemphasis *preemphasis;
int preemphasized_audio_signal_get_data(size_t offset, size_t length, float *out_ptr) {
    int r = preemphasis->get_data(offset, length, out_ptr);
    return r;
}

float features_matrix_buffer[49 * 13];

// callback that gets invoked when TARGET_AUDIO_BUFFER is full
void run_classifier() {
    Timer t;
    t.start();

    raw_audio_signal.total_length = 16000;
    preemphasized_audio_signal.total_length = 16000;

    int ret;

    matrix_t features_matrix(49, 13, features_matrix_buffer);

    // and run the MFCC extraction (using 32 rather than 40 filters here to optimize speed on embedded)
    ret = speechpy::feature::mfcc(&features_matrix, &preemphasized_audio_signal,
        16000, 0.02f, 0.02f, 13, 32, 512);
    if (ret != EIDSP_OK) {
        printf("ERR: MFCC failed (%d)\n", ret);
        return;
    }

    t.stop();

    printf("mfcc done in %d ms.\n", t.read_ms());

    t.reset();
    t.start();

    // cepstral mean and variance normalization
    ret = speechpy::processing::cmvnw(&features_matrix, 101, true);
    if (ret != EIDSP_OK) {
        printf("ERR: cmvnw failed (%d)\n", ret);
        return;
    }

    t.stop();

    printf("cmvnw done in %d ms.\n", t.read_ms());

    Timer classify_timer;
    classify_timer.start();

    // print MFCC features from the signal
    print_matrix("mfcc_features", &features_matrix);

    ei_impulse_result_t result;

    int r = run_classifier(features_matrix.buffer, features_matrix.cols * features_matrix.rows, &result);
    printf("Classifier done %d\n", r);

    // classify_timer.stop();
    // printf("classifier took %d ms.\n", classify_timer.read_ms());

    printf("Classification result:\n");
    for (size_t ix = 0; ix < INFERENCING_LABEL_COUNT; ix++) {
        printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
    }
}

void start_recording() {
    ei_microphone_sample_start();

    printf("Done recording\n");

    run_classifier();
}


int main() {
    printf("Hello from the B-L475E-IOT01A microphone demo, have BD? %p\n", bd);

    uint32_t frequency = 16000;
    size_t signal_length = 16000;

    printf("BD init returned %d\n", bd->init());

    // create a structure to retrieve data from the signal
    raw_audio_signal.total_length = signal_length;
    raw_audio_signal.get_data = &raw_audio_signal_get_data;

    // preemphasis class to preprocess the audio...
    preemphasis = new class speechpy::processing::preemphasis(&raw_audio_signal, 1, 0.98f);

    preemphasized_audio_signal.total_length = raw_audio_signal.total_length;
    preemphasized_audio_signal.get_data = &preemphasized_audio_signal_get_data;

    // calculate the size of the MFCC matrix
    matrix_size_t out_matrix_size =
        speechpy::feature::calculate_mfcc_buffer_size(signal_length, frequency, 0.02f, 0.02f, 13);
    printf("out_matrix_size is %hu x %hu\n", out_matrix_size.rows, out_matrix_size.cols);

    // features_matrix = new matrix_t(out_matrix_size.rows, out_matrix_size.cols);

    ei_microphone_init();

    printf("Press the BLUE button to record a message\n");

    // hit the blue button to record a message
    static InterruptIn btn(BUTTON1);
    btn.fall(ev_queue.event(&start_recording));

    ev_queue.dispatch_forever();
}

static void print_matrix(const char *name, matrix_t *matrix) {
    printf("%s: [\n", name);
    for (uint16_t rx = 0; rx < matrix->rows; rx++) {
        printf("    [ ");
        for (uint16_t cx = 0; cx < matrix->cols; cx++) {
            printf("%f ", matrix->buffer[rx * matrix->cols + cx]);
            if (cx != matrix->cols - 1) {
                printf(", ");
            }
        }
        printf("] ");
        if (rx != matrix->rows - 1) {
            printf(", ");
        }
        printf("\n");
    }
    printf("]\n");
}
