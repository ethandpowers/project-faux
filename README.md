# Project Faux

I haven't decided on a name for this mini project yet, but I find myself in
need of a test modbus & bacnet server.  Since I'm trying to get better at C and
C++, I decided to go ahead and write it myself.  Here goes nothing!

My basic requirements are this:

1. The user provides a yaml file containing the mapping for field names,
   default values, and modbus/bacnet registers.
2. This program serves those fields on the respective platforms as configured.
3. The user can change the values in real-time via a scriptable interface.
4. For now, this will be strictly read-only from the perspective of the
   modbus/bacnet client.
5. Eventually, I want it to be able to "replay" events given some time-series data.

## Dependencies

- Docker (if you want to easily spin up a virtual network)
- yaml-cpp
- libmodbus (install via system package manager)
- bacnet-stack
- CLI11

> [!NOTE]
> This uses zero-based registers across the board, so the field map and
> request register numbers must match.  All multi-register types are
> represented in big-endian format.

## Executing Commands

You can start the server via the included `Dockerfile`, `compose.yaml`, or
calling the binary directly.  The same binary can be used as a server or
client, so once you have it build, start a server with `faux serve
<path/to-field/map>`.

The following flags are available for the server:

| Flag | Description |
| ---- | ----------- |
| -p, --port | Specify server control port (TCP) |
| -m, --modbus | Start the Modbus server |
| -b, --bacnet | Start the BACnet server |
| --modbus-port | Specify the Modbus port |
| --modbus-connections | Set the max modbus connections |

To maintain extreme scriptability (Is that a word?  It is now!), faux fully
supports using netcat (or any other TCP client) for controlling the server. You
can use it to create interactive sessions with `nc <host> <port>` or you can
send a single command with `echo "<your command>" | nc <host> <port> -N`.

However, if you so desire, the faux binary can also serve as a client,
executing single commands, or with a REPL.  If you call `faux` without the
`serve` subcommand, it will act as a client with the following arguments:

| Flag | Description |
| ---- | ----------- |
| -p, --port | Specify server control port (TCP) |
| -s, --server | Target server for client |
| -r, --repl | Open a REPL instead of sending a command |

## Creating A Virtual Network With Docker

I included an example Dockerfile and compose.yaml file which can be used to
instantly set up and tear down a virtual network with Modbus and BACnet devices.
You can also see some example field maps in the `/maps` directory.

To spin up a virtual network, prepare field maps for the devices you wish to
simulate, or use the included examples.  Then, update the included
`compose.yaml` file to include your device maps, or create one from scratch. To
start the containers, run: `docker compose up`.  Then, to execute commands,
connect to the exposed ports defined in `compose.yaml`, for example `nc
localhost 14921` or `mbpoll localhost -t 4:float -p 5021 -B -0 -r 3110`.
