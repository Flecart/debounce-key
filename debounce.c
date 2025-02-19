#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <syslog.h>

#define DEBOUNCE_THRESHOLD_NS 100000000ULL  // 100 ms in nanoseconds

// Change this to your keyboard's evdev device:
#define INPUT_DEVICE "/dev/input/event3"
int main(void) {
    int fd, uinput_fd;
    struct input_event ev;
    struct uinput_setup usetup;
    /* Open the input device (keyboard) */
    fd = open(INPUT_DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input device");
        return EXIT_FAILURE;
    }
    
    /* Grab the device exclusively so that no other process sees its events */
    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        perror("Failed to grab the input device");
        close(fd);
        return EXIT_FAILURE;
    }
    
    /* Open uinput device to re-inject filtered events */
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        perror("Failed to open uinput device");
        ioctl(fd, EVIOCGRAB, 0);
        close(fd);
        return EXIT_FAILURE;
    }
    
    /* Enable key and sync events for our uinput device */
    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY) < 0) {
        perror("UI_SET_EVBIT EV_KEY");
        goto cleanup;
    }
    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN) < 0) {
        perror("UI_SET_EVBIT EV_SYN");
        goto cleanup;
    }

    for (int i = 0; i < KEY_MAX; i++) {
        if (ioctl(uinput_fd, UI_SET_KEYBIT, i) < 0) {
            perror("UI_SET_KEYBIT");
            goto cleanup;
        }
    }
    
    /* Enable the KEY_A event.
       (You can add additional keys if needed.) */
    // if (ioctl(uinput_fd, UI_SET_KEYBIT, KEY_A) < 0) {
    //     perror("UI_SET_KEYBIT KEY_A");
    //     goto cleanup;
    // }
    
    /* Setup the uinput device */
    memset(&usetup, 0, sizeof(usetup));
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "debounce-keyboard");
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    
    if (ioctl(uinput_fd, UI_DEV_SETUP, &usetup) < 0) {
        perror("UI_DEV_SETUP");
        goto cleanup;
    }
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("UI_DEV_CREATE");
        goto cleanup;
    }
    
    syslog(LOG_INFO, "Debounce user-space program running. Filtering duplicate 'a' key presses...");
    // printf("Debounce user-space program running. Filtering duplicate 'a' key presses...\n");
    
    /* Variables for debounce logic */
    struct timespec last_a_time = {0, 0};
    
    /* Main loop: read events, filter, and re-inject */
    while (1) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n != sizeof(ev)) {
            perror("Failed to read input event");
            break;
        }
    
        /* If this is a key event for KEY_A and it is a press (value == 1) */
        if (ev.type == EV_KEY && ev.code == KEY_A && ev.value == 1) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t now_ns = now.tv_sec * 1000000000ULL + now.tv_nsec;
            uint64_t last_ns = last_a_time.tv_sec * 1000000000ULL + last_a_time.tv_nsec;
            
            // printf("Current displacement %lu\n", (now_ns - last_ns) / 1000000);

            /* Check if the time difference is less than 100ms */
            if (last_ns != 0 && (now_ns - last_ns < DEBOUNCE_THRESHOLD_NS)) {
                syslog(LOG_INFO, "Filtered duplicate KEY_A press");
                /* Skip re-injecting this event */
                continue;
            }
            /* Update last_a_time for KEY_A */
            last_a_time = now;
        }
        
        /* Write the (possibly unfiltered) event to the uinput device */
        if (write(uinput_fd, &ev, sizeof(ev)) < 0) {
            perror("Failed to write event to uinput");
            break;
        }
    }
    
cleanup:
    /* Cleanup: destroy the uinput device and release the grab */
    if (ioctl(uinput_fd, UI_DEV_DESTROY) < 0) {
        perror("UI_DEV_DESTROY");
    }
    close(uinput_fd);
    ioctl(fd, EVIOCGRAB, 0);
    close(fd);
    return EXIT_SUCCESS;
}
