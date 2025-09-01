in our playpretend world we are creating what commands do we envision that will allow us to create start and stop vm.  we will have to configure memory virtual disks and enable UART ?

###### ChatGPT said:

Thought for 14s

Love it—let’s sketch a tight, POSIX-y **command set** your `vim-cmd` can send to `hostd` to cover create/start/stop, memory/disk config, and UART/serial.

I’ll keep it simple, consistent with your current **line-protocol** (`200 OK …` / `400 ERR …`) and your **key=value** parser.

# Wire conventions (quick)

- One command per line.
- Arguments are `key=value` pairs, space-separated.
- Values may be quoted `"like this"`; escape `\"` and `\\`.
- Booleans: `1|0` or `true|false`.
- Sizes accept suffixes: `K,M,G,T` (base-2).
- Identify a VM with **`vm=`** which can be a numeric id **or** a name (name must be unique).

# Core lifecycle

```
VERSION
PING
VM.LIST
VM.INFO vm=<id|name>
VM.CREATE name=<name> mem=<MB> cpu=<vcpus> [guest=os_type] [firmware=bios|efi] \
          [disk.size=<size>] [disk.thin=1] [net.pg="<portgroup>"] [notes="..."]
VM.DELETE vm=<id|name> [keep-disks=0|1]
VM.START  vm=<id|name>
VM.STOP   vm=<id|name> [hard=0|1]     # hard=1 = power-off
VM.RESET  vm=<id|name>
VM.SUSPEND vm=<id|name>
```

### Examples

```
VM.CREATE name=demo mem=2048 cpu=2 disk.size=20G disk.thin=1 net.pg="VM Network"
VM.START vm=demo
VM.INFO vm=demo
VM.STOP vm=demo hard=1
VM.DELETE vm=demo keep-disks=0
```

# Reconfigure (power-off unless hot-add is supported)

```
VM.SET vm=<id|name> mem=<MB> [cpu=<vcpus>] [name="<newname>"] [notes="..."]
```

- Later: add `hot=1` to request hot-add if available; server can reply `409 ERR requires poweroff` if not.

# Disks

Two paths: **implicit on create** (`disk.size=`), and **explicit device ops** below.

```
DISK.ADD    vm=<id|name> size=<N[M|G|T]> [thin=1|0] [controller=lsilogic|nvme|sata] \
           [unit=<0..15>] [name="<vmdk-name>"] [datastore=<ds>]
DISK.ATTACH vm=<id|name> path="[<ds>] <folder>/<name>.vmdk" \
           [controller=lsilogic|nvme|sata] [unit=<0..>] 
DISK.DETACH vm=<id|name> [path=...] [controller=...] [unit=...]
DISK.LIST   vm=<id|name>
```

### Examples

```
DISK.ADD vm=demo size=40G thin=1 controller=lsilogic unit=0
DISK.ATTACH vm=demo path="[datastore1] demo/demo-data.vmdk" controller=sata unit=1
DISK.DETACH vm=demo controller=lsilogic unit=0
```

# Networking (minimal)

```
NIC.ADD    vm=<id|name> [type=e1000e|vmxnet3] [pg="<portgroup>"] [mac=auto|xx:xx:..]
NIC.DEL    vm=<id|name> [index=<n>]           # index from NIC.LIST
NIC.LIST   vm=<id|name>
```

### Example

```
NIC.ADD vm=demo type=vmxnet3 pg="VM Network"
```

# UART / Serial (enable, backend)

Backends you can support today in the strawman:

- `file:<path>`  (append=0|1)
- `unix:<path>`  (UNIX domain socket; mode=server|client)
- `tcp:<host:port>` (optional later)

```
UART.ADD    vm=<id|name> backend=<file:PATH|unix:PATH|tcp:HOST:PORT> \
           [server=1|0] [append=1|0] [pipe=0|1] [index=<n>]
UART.DEL    vm=<id|name> [index=<n>]
UART.LIST   vm=<id|name>
```

### Examples

```
UART.ADD vm=demo backend=unix:/var/run/demo.serial server=1
UART.ADD vm=demo backend=file:/var/log/demo-serial.log append=1
UART.LIST vm=demo
```

# Datastores (read-only, handy for UX)

```
DS.LIST                     # name, type, capacity, free
DS.INFO name=<ds>
```