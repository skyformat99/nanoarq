nanoarq [![](https://travis-ci.org/charlesnicholson/nanoarq.svg?branch=master)](https://travis-ci.org/charlesnicholson/nanoarq) [![](https://img.shields.io/badge/license-public_domain-brightgreen.svg)](https://github.com/charlesnicholson/nanoarq/blob/master/LICENSE)
-

### **current nanoarq status:**
**nanoarq is under active development and not yet ready for general release!**

----

nanoarq is a tiny implementation of the [Selective Repeat ARQ](https://en.wikipedia.org/wiki/Selective_Repeat_ARQ) protocol, suitable for embedded systems.

The nanoarq runtime is written in 99% ANSI C90 and delivered as a single header file. There is also an exhaustive suite of unit and functional tests, as well as usage examples, all in C++14.

nanoarq compiles to roughly 5KB of ARM Thumb2 object code, and its runtime memory footprint is determined by the user via runtime configuration. nanoarq performs no memory allocation, operating entirely in the user-provided seat it gets at initialization.

### Motivation
Sometimes there's a need for reliable communications over strange or nontraditional transports that don't come with reliability guarantees. There don't appear to be easily-reused embeddable open-source implementations of reliable protocols, so I wrote one.

Interesting places I've wished I'd had nanoarq:
* Inter-chip UART with only TX and RX signals.
* USB, where entire HID reports would get lost due to signal interference on the wire.
* Bidirectional audio jack modem with high latency and low bandwidth.
* Incoming buffer exhaustion in firmware caused newly-arriving data to be dropped.

**If your OS provides TCP/IP to you, please use it!** nanoarq is not meant to replace TCP/IP in any general sense. It is meant as a reliability layer for nontraditional transports! If you absolutely can't use TCP/IP or take advantage of natural reliability your transport may offer, then maybe nanoarq can help.

### Features
* Reliability over an unreliable transport. No duplication, no dropped data, always in order.
* Optional message subdivision, (hopefully) decreasing the size of each retransmission.
* Optional frame integrity via user-defined checksum. A software [CRC32](https://en.wikipedia.org/wiki/Cyclic_redundancy_check) is provided.
* Unambiguous framing of payloads using [COBS](https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing) encoding.
* Basic flow control and [Silly Window Syndrome](https://en.wikipedia.org/wiki/Silly_window_syndrome) avoidance.
* Optional stateful connections (like TCP/IP).
* Environment agnostic; doesn't depend on a specific OS or transport.

### Usage
Since nanoarq by design knows nothing about the transport itself, it is up to the user to 'glue' nanoarq to a transmitter and receiver. This leads to an API that is split into 'frontend' and 'backend' calls.

The initialization:
* `arq_required_size()` tells you how much memory you need to provide to a nanoarq instance based on the configuration values you pass to it.
* `arq_init()` instantiates a nanoarq instance in the user-provided memory seat.

The frontend API is modeled after [Berkeley sockets](https://en.wikipedia.org/wiki/Berkeley_sockets#Socket_API_functions):
* `arq_connect()` begins a 3-way handshake with the peer, if enabled.
* `arq_close()` closes an established connection to the peer, if enabled.
* `arq_recv()` drains any pending data from receive window.
* `arq_send()` loads data into the send window for reliable transmission.
* `arq_flush()` flags a pending small / partial message for transmission.

The backend API is designed to be called as infrequently as possible without wasting time or cycles:
* `arq_backend_poll()` steps timers, manages windows, and returns the state of nanoarq.
* `arq_backend_send_ptr_get()` and `arq_backend_send_ptr_release()` exposes outgoing data for transmission.
* `arq_backend_recv_fill()` receives incoming data from the peer into nanoarq.


### Integration
`arq.h` is written in the style of the excellent [STB libraries](https://github.com/nothings/stb), and contains both the nanoarq API and the nanoarq implementation. `arq.h` will not compile if any of the configuration flags are not set, but it attempts to steer you towards success with helpful error messages. `arq.h` compiles cleanly as C90, C99, C11, C++03, C++11, and C++14 with extremely pedantic warnings enabled.

Here is an example of integrating `arq.h` into your application:

```c
/* arq_in_my_project.h */
#pragma once
#define ARQ_USE_C_STDLIB 1
#define ARQ_COMPILE_CRC32 1
#define ARQ_LITTLE_ENDIAN_CPU 1
#define ARQ_ASSERTS_ENABLED 1
#define ARQ_USE_CONNECTIONS 1
#include "arq.h"
```

```c
/* arq_in_my_project.c[pp] */
#define ARQ_IMPLEMENTATION
#include "arq_in_my_project.h"
```

When using nanoarq in your project, you should use it through your `#include "arq_in_my_project.h"` wrapper, so your configuration flags remain consistent in your project.

### More

Check out the examples, and read the [paper](https://github.com/charlesnicholson/nanoarq/blob/window/doc/nanoarq.pdf).

