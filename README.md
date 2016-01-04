# ccnx-fwdharness

## Overview

This is a project to stress test CCNx-compliant forwarders. The test
consists of three elements: a packet sender, receiver, and forwarder. 
The sender is connected to the forwarder to push packets through
the forwarder at a given rate (currently: as fast as possible)
and the receiver is connected to the forwarder to receive packets and 
respond with pre-computed packets. This general setup is shown below.

```
     sender window
    +-+-+-+-+-+-+-+
    | | | | | | | |
    +---+-+++-+---+
+--------+   |    +-----------+       +----------+
|        +---+---->           +------->          |
| Sender |        | Forwarder |       | Receiver |
|        <--------+           <-------+          |
+--------+        +-----------+       +----------+
```

The sender controls the rate at which packets are sent by windowing.
That is, packets are sent and inserted into the window until the 
window is full. The sender then attempts to receive packets. To keep
the forwarder busy, sending is always given priority over receiving. 
This means that the window is usually full for a given packet load. 

## Packet Generation

Packets are generated using the [ccnx-pktgen tool](https://github.com/chris-wood/ccnx-pktgen).
This can generate interest and content object pairs by specifying the 
name size, name component length, and the content object payload size. 
The tool will create a pair of binary files containing a sequence of wire-encoded
interest and content objects. The sender and receiver use these files to 
build an indexes that allow fast lookup and retrieval. 

## Forwarder Connections

Currently, only UDP connections are supported for the PARC Metis forwarder.

