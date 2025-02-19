#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/timekeeping.h>

/* 200 ms expressed in nanoseconds */
#define DEBOUNCE_THRESHOLD_NS 200000000ULL

static u64 last_a_time = 0;
const int KEY_A = 64353;  // seen from dmesg logs

/* Keyboard notifier callback */
static int debounce_keyboard_notifier(struct notifier_block *nb,
                                      unsigned long action, void *data)
{
    struct keyboard_notifier_param *param = data;
    u64 now;

    /* Log every event that reaches this notifier */
    /* We only care about key events */
    if (action != KBD_KEYSYM)
        return NOTIFY_OK;

    /* Process only key press events (not releases) */
    if (!param->down)
        return NOTIFY_OK;

    /* Check if the key is 'a' (KEY_A is defined in <linux/input.h>) */
    if (param->value != KEY_A) {
        return NOTIFY_OK;
    }
    
    now = ktime_get_ns();

    pr_info("Debounce module: time in ms time=%llu", (now - last_a_time)/1000000);

    if (now - last_a_time < DEBOUNCE_THRESHOLD_NS) {
        pr_info("Debounce module: Filtering duplicate 'a' key press\n");
        return NOTIFY_STOP;  /* Block the event */
    }

    last_a_time = now;
    return NOTIFY_OK;
}

/* Notifier block registration */
static struct notifier_block debounce_nb = {
    .notifier_call = debounce_keyboard_notifier,
};

static int __init debounce_init(void)
{
    int ret;
    pr_info("Debounce module loaded\n");
    ret = register_keyboard_notifier(&debounce_nb);
    if (ret) {
        pr_err("Debounce module: Failed to register keyboard notifier\n");
        return ret;
    }
    return 0;
}

static void __exit debounce_exit(void)
{
    unregister_keyboard_notifier(&debounce_nb);
    pr_info("Debounce module unloaded\n");
}

module_init(debounce_init);
module_exit(debounce_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xuanqiang 'Angelo' Huang");
MODULE_DESCRIPTION("Kernel module to debounce duplicate 'a' key presses within 100ms");
