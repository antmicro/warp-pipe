# Using the library

The library itself is called `warp-pipe`. You can build it using the instructions provided [in the "Basic usage" chapter](#basics).
Its API revolves around 'servers' (connection pools) and 'clients' (connections).

[readme]: https://github.com/antmicro/warp-pipe#readme
You can then link the library to your project using pkg-config as usual, like:
```
LDFLAGS += $(pkg-config --libs warp-pipe)
CFLAGS += $(pkg-config --cflags warp-pipe)
```

The main library headers are as follows:
```c
#include <warppipe/server.h>  /* managing connection pools */
#include <warppipe/client.h>  /* managing BARs, issuing or responding to PCIe requests */
#include <warppipe/proto.h>   /* low-level protocol handling */
```

Because PCIe is mostly a bidirectional network, any device (including the root complex) can access any other device;
therefore the API and implementation is necessarily shared across the peripheral side and the central side.
Be cautious, because not every API use will make sense in the real world, or even worse, it might make perfect sense,
but will be at least somewhat unexpected (like a simple peripheral querying the config space of the root complex,
or reconfiguring the bridges along the way).


## Connection pool

You can create a connection pool by declaring a `struct warppipe_server_t`,
filling in the relevant fields, like in the [memory-mock example][memmock].
Then you need to initialize the internals with `warppipe_server_create`.
The example is an excellent peripheral implementation to look at in order to learn the basics.

[memmock]: https://github.com/antmicro/warp-pipe/blob/main/memory-mock/main.c

A connection pool is capable of either listening on a port or connecting to a remote address,
and managing the resulting connections on a basic level.
In order to do anything useful with the pool, you need to register an `accept` callback
initializing the particular incoming (or outgoing) connections with `warppipe_server_register_accept_cb`.
Most of the library functions (including this one) take callbacks that accept an additional `private_data` parameter,
just like `arg` in `qsort(3)` or `pthread_create(3)`, allowing to pass additional data (a closure) to the callback.
In this case, `private_data` is taken straight from the pool structure.

When set up correctly, you can call the `warppipe_server_loop` function at your convenience.

Example:
```c
warppipe_server_t pool = {
    .port = "2115",
    .listen = true,
    .private_data = &some_internal_structure,
};

warppipe_server_create(&pool);
warppipe_server_register_accept_cb(&pool, accept_cb);

while (!server.quit) {
    // ... somewhere in the main loop ...
    warppipe_server_loop(&pool);
    // ...
}
```


## PCIe basics

Every connection either managed by a pool or manually, in order to be accessed, needs to have a BAR registered.
You can do this in the accept callback or elsewhere, by calling `warppipe_register_bar`,
supplying the BAR, size, index and optional read/write callbacks.
Missing callbacks will generate warnings and leave incoming packets without a response
(writes will get discarded and reads will currently 'hang').

Example:
```c
int fd = ...;

warppipe_client_t conn;
warppipe_client_create(&conn, fd);
warppipe_register_bar(&client, 0x10000, 0x8000, bar_idx, read_cb, NULL);
```

On each connection you can issue a single read or write operation, by calling `warppipe_read` or `warppipe_write`.
They will submit an operation and return immediately.
The `warppipe_read` function takes a callback function that gets called when the completion packet arrives.

Example:

```c
warppipe_read(&conn, bar_idx, 0x3400, 0x200, read_handler);
warppipe_write(&conn, bar_idx, 0x3500, "12345678", 8);
```


## Configuration space

You can access the PCIe configuration space with a very similar API to plain read and write requests.

Example:
```c
warppipe_config0_read(&conn, 0x30, 0x10, completion_cb);
warppipe_config0_write(&conn, 0x40, "1234", 4);
```

On the peripheral side you can implement the configuration space with a simple API as well:
```c
warppipe_register_config0_read_cb(&conn, config_read_cb);
warppipe_register_config0_write_cb(&conn, config_write_cb);
```

(wire-format)=
## Wire format

The format is just a single `\x02` (DLLP) or `\x03` (TLP) byte with the relevant packet contents appended,
which means a structure as follows:

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Protocol = 2  |   DLLP Type   |  (type-dependent contents...  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |      ...)     |           16-bit CRC            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Protocol = 3  |0 0 0 0|  TLP Sequence Number  | Fmt |   Type  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |T|     |T|A|L|T|T|E|   |   |                   |               :
   |9|  TC |8|t|N|H|D|P|Att| AT|       Length      |               :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+               :
   :                              TLP payload...                   :
   :                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           32-bit LCRC                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```