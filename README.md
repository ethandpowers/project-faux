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

## Demo

[![Demo: faux devices running under docker compose, with values changed from
the faux client and read live over Modbus (mbpoll) and BACnet
(Wacnet)](docs/demo.png)](docs/demo.mp4)

A virtual network started with `docker compose up`, with field values changed
live via the faux client and picked up over Modbus (mbpoll) and BACnet
(Wacnet).  Click the image to watch the video.

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

## Basic Usage

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

> [!NOTE]
> I am using the `bacnet-stack` library for all BACnet operations, so if
> you want to customize the BACnet options (interface, port, etc.) you
> can use the env variables supported by that library, ex.
> `BACNET_IFACE=eth0 faux serve ./maps/sample1.yaml`

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

### Commands

Commands are sent as a single line of text over the control port, either with
the `faux` client or any TCP client like netcat.  Arguments are separated by
whitespace.

| Command | Description |
| ------- | ----------- |
| `help` | Show the help message |
| `field list` | List every field in the field map |
| `field get <name>` | Get the current value of a field |
| `field set <name> <value>` | Set the value of a field |
| `modbus start` | Start the Modbus server |
| `modbus stop` | Stop the Modbus server |
| `modbus status` | Get the status of the Modbus server |
| `modbus get port` | Get the port the Modbus server listens on |
| `modbus get max-connections` | Get the max number of Modbus connections |
| `modbus set max-connections <n>` | Set the max number of Modbus connections |
| `bacnet start` | Start the BACnet server |
| `bacnet stop` | Stop the BACnet server |
| `bacnet status` | Get the status of the BACnet server |

For `field set`, the value is parsed according to the field's type in the
field map.  Booleans must be `true` or `false`, and since arguments are split
on whitespace, string values can't contain spaces.

For example:

```sh
faux field list
faux field set da_temp_sp 55.5
faux -s 172.28.0.13 field get da_temp_sp
echo "field get da_temp" | nc localhost 1492 -N
```

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
