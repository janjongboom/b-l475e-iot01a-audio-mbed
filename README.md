# Mbed OS microphone example for the B-L475E-IOT01A development board

This is an example application for the MP34DT01 MEMS microphone on the ST IoT Discovery Kit (also known as the [DISCO-L475VG-IOT01A](https://os.mbed.com/platforms/ST-Discovery-L475E-IOT01A/) board) for Mbed OS 5. It records two seconds of data from the microphone, creates a WAV file, and outputs the binary file over serial. The port is based on the [FP-AI-SENSING1](https://www.st.com/en/embedded-software/fp-ai-sensing1.html) function pack built by ST. This function pack contains a newer version of the board support package (BSP) for this board that has a microphone driver.

This example application was tested with the ST B-L475E-IOT01A2 (European version) development board, compiled with GCC ARM 6.3.1.

## How to build and use this application

1. Install [Mbed CLI](https://github.com/ARMmbed/mbed-cli) and its dependencies.
1. Clone this repository:

    ```
    $ mbed import https://github.com/janjongboom/b-l475e-iot01a-audio-mbed
    ```

1. Build and flash the project:

    ```
    $ cd b-l475e-iot01a-audio-mbed
    $ mbed compile -t GCC_ARM -m DISCO_L475VG_IOT01A --profile=debug -f
    ```

1. Open a serial monitor on baud rate 115200 to see the output.

## Recording and playing back audio

With the serial monitor attached:

1. Hit the blue button to start recording.
1. After two seconds a binary file in HEX format will be displayed (in one line with no spaces).
1. Copy the full file, and store it in `raw-audio.txt` in the `b-l475e-iot01a-audio-mbed` folder.
1. Open a terminal or command prompt and navigate to the `b-l475e-iot01a-audio-mbed` folder.
1. Turn the raw audio into a binary file via (requires Node.js):

    ```
    cat raw-audio.txt | node converter/hex-to-buffer.js test.wav
    ```

1. Open `test.wav` through your favourite audio application.

## Machine learning

Want to use the microphone on this board for something useful? At [Edge Impulse](https://www.edgeimpulse.com) we enable developers to create the next generation of intelligent device solutions with embedded Machine Learning. Through our platform you can build, test and deploy ML models that run on the smallest devices (including the B-L475E-IOT01A development board).

## License

All files are licensed under BSD 3-Clause license.
