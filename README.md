# Debouncer


## Problem
- My keyboard has a hardware issue that makes it input the sole key "A" multiple times some time. Sometimes double, sometimes triple. It is a random event :).

## Solution
- One possible solution was to change Ubuntu settings to ignore the repeated keys if it was under a certain value
  - Drawback: it would have applied for every single key-stroke
  - My problem was just with the key "A"
- I considered first writing an energy efficient kernel module to handle this problem.
- But it turns out I would had to interface with the hardware drivers, which is much more delicate.

So I implemented this version of a program that runs in user space.
It was quite efficient to implement, thanks to ChatGPT.
I'll link [here](https://chatgpt.com/share/67b5b19c-ed8c-8009-9e27-9f23fc6092e3) the conversation I had. The whole project took 1h.7m.


