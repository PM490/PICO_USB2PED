/* 
=======================================================
PICO Serial-USB to Stepper PED (Pulse Enable Direction)
Version 1.01

PM490
=======================================================

Code Provided as-is, no warranties whatsoever.

Character Command   Char	Hex
===============================
One Turn		    T	    54
Cancel Turn         t       74
Pulse Continuos	On	P	    50
Pulse Continuos Off	p	    70
Enable set High		E	    45
Enable set Low		e	    65
Direction set High	D	    44
Direction set Low	d	    64
LED set High		L	    4C
LED se Low		    l	    6C
==============================

Pulses per Turn Setting
Total Pulses = 200 x 2 ^ X
--------------------------
Pulses      Char    Hex
200         0       0x30
400         1       0x31
800         2       0x32
1600        3       0x33
3200        4       0x34
6400        5       0x35
12800       6       0x36
25600       7       0x37
51200       8       0x38

Pulses Frequency Setting
------------------------
Hz	        Char    Hex
7936.51	    *	    2A
7042.25	    &	    26
6024.01	    ^	    5E
5000	    %	    25
4000	    $	    24
2994.01	    #	    23
2000	    @	    40

=======================================================
1.00 Initial Release
1.01 Set definitions to simplify reasignment of Ports
=======================================================

*/

#include <stdio.h>
#include "pico/stdlib.h"

#define LED_CTRL_GP 19
#define STEPPER_ENABLE_GP 18
#define STEPPER_DIRECTION_GP 17
#define STEPPER_PULSE_GP 16


using namespace std;

int main() {
 
    // LED Init
    const uint8_t LED_PIN = PICO_DEFAULT_LED_PIN;

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // LED Blink Alive
    gpio_put(LED_PIN, 1);
    busy_wait_ms (250);

    gpio_put(LED_PIN, 0);
    busy_wait_ms (250);

    gpio_put(LED_PIN, 1);
    busy_wait_ms (250);

    gpio_put(LED_PIN, 0);
    busy_wait_ms (250);

    stdio_init_all(); // Init STD I/O for printing over serial

    // PED Variables
    bool stepper_turn = false;
    bool stepper_step = false;
    bool stepper_enable = false;
    bool stepper_direction = false;
    bool led_enable = false;

    uint64_t time_next = 0;
    uint32_t turn_counter = 0;

    // Start up Defaults - 8KHz and 1600 steps per turn
    uint64_t half_period = 63;
    uint32_t stepper_steps_per_turn = 1600;

    int rec_char = 0;
    bool square_pulse_out = false;


    //IO Inits

    // Pulse Port
    gpio_init(STEPPER_PULSE_GP);
    gpio_set_dir(STEPPER_PULSE_GP, GPIO_OUT);

    // Direction Port
    gpio_init(STEPPER_DIRECTION_GP);
    gpio_set_dir(STEPPER_DIRECTION_GP, GPIO_OUT);

    // Enable Port
    gpio_init(STEPPER_ENABLE_GP);
    gpio_set_dir(STEPPER_ENABLE_GP, GPIO_OUT);

    // LED Port
    gpio_init(LED_CTRL_GP);
    gpio_set_dir(LED_CTRL_GP, GPIO_OUT);

    // Initialize Serial USB
    stdio_usb_init(); // init usb only
    while(!stdio_usb_connected()); // wait until USB connection
    printf("USB-Serial to PED\n");
  
    // Edge Time Arming
    time_next = to_us_since_boot (get_absolute_time());
    time_next = time_next + half_period;

    while(true){ // Square Waveform Loop

        // While in a Turn Command, update counter and flags
        if(stepper_turn) {
            if (turn_counter == 0) {
                stepper_turn = false;
                stepper_step = false;
            }
            else {
               stepper_step = true;
            }
         }

        // Stage pulse Output
        square_pulse_out = (!square_pulse_out) & stepper_step; 
        if (turn_counter > 0) {
            turn_counter--;
        }

        // Wait until Edge Time arrives (previously armed)
        busy_wait_until(from_us_since_boot(time_next)); // This is the edge, my only friend... the edge (no end here) 
        time_next = time_next + half_period; // Arming next Edge

        // **** PROCESSING BLOCK ****

        // Post GPIO Outputs
        gpio_put(STEPPER_PULSE_GP,square_pulse_out);
        gpio_put(STEPPER_DIRECTION_GP,stepper_direction);
        gpio_put(STEPPER_ENABLE_GP,stepper_enable);
        gpio_put(LED_CTRL_GP,led_enable);

        //  Check for character waiting
        rec_char = getchar_timeout_us(0);
        if (rec_char == -1) {
                gpio_put(LED_PIN, 0);
        }
        else {
                gpio_put(LED_PIN, 1);

//          Uncomment for Echo, use only for testing as it may affect pulse timming
//          printf("%02x\n",rec_char);

            // Parse Character Received and corresponding actions
            switch ( rec_char ) {
                case 0x64 : //d     // *** Default at Statup
                    stepper_direction = false;
                    break;
                case 0x44 : //D
                    stepper_direction = true;
                    break;
                case 0x65 : //e     // *** Default at Statup
                    stepper_enable = false;
                    break;
                case 0x45 : //E
                    stepper_enable = true;
                    break;
                case 0x6c : //l     // *** Default at Statup
                    led_enable = false;
                    break;
                case 0x4c : //L
                    led_enable = true;
                    break;
                case 0x70 : //p     // *** Default at Statup
                    stepper_step = false;
                    break;
                case 0x50 : //P
                    stepper_step = true;
                    break;
                case 0x74 : //t - Cancel Turn
                    stepper_turn = false;
                    stepper_step = false;
                    turn_counter = 0;
                    break;
                case 0x54 : //T
                    if (!stepper_turn) {
                        stepper_turn = true;
                        turn_counter = stepper_steps_per_turn; // Counter is double the number of steps, 3200 = 1600 steps
                    }
                    break;
                // Define Pulses Per Turn
                case 0x30 : //0
                    stepper_steps_per_turn = 200;
                    break;
                case 0x31 : //1
                    stepper_steps_per_turn = 400;
                    break;
                case 0x32 : //2
                    stepper_steps_per_turn = 800;
                    break;
                case 0x33 : //3
                    stepper_steps_per_turn = 1600;  // *** Default at Statup
                    break;
                case 0x34 : //4
                    stepper_steps_per_turn = 3200;
                    break;
                case 0x35 : //5
                    stepper_steps_per_turn = 6400;
                    break;
                case 0x36 : //6S
                    stepper_steps_per_turn = 12800;
                    break;
                case 0x37 : //7
                    stepper_steps_per_turn = 25600;
                    break;
                case 0x38 : //8
                    stepper_steps_per_turn = 51200;
                    break;
                //Frequency Setting
                case 0x40 : //@
                    half_period = 250; // 2.0 KHz
                    break;
                case 0x23 : //#
                    half_period = 167; // 2.99 KHz
                    break;
               case 0x24 : //$
                    half_period = 125; // 4.0 KHz
                    break;
               case 0x25 : //%
                    half_period = 100; // 5.0 KHz
                    break;
               case 0x5e : //^
                    half_period = 83; // 6.02 KHz
                    break;
               case 0x26 : //&
                    half_period = 71; // 7.04 KHz
                    break;
               case 0x2A : //*
                    half_period = 63; // 7.94 KHz *** Default at Statup
                    break;
            }
        }
        // **** END OF BLOCK ****
    }
}
