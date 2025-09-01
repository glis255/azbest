#if SERIAL_NUMBER == 1
    #define RELAY_PIN 17
    #define LED_R_PIN 25
    #define LED_G_PIN 27
    #define LED_B_PIN 26
    #define ENC_BTN_PIN 13
    #define ENC_A_PIN 16
    #define ENC_B_PIN 14
#else
    #define RELAY_PIN 4
    #define LED_R_PIN 25
    #define LED_G_PIN 27
    #define LED_B_PIN 26
    #define ENC_BTN_PIN 13
    #define ENC_A_PIN 14
    #define ENC_B_PIN 16
#endif