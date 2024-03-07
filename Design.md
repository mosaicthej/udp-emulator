---
Name: Mark Jia
NSID: mij623
Student Number: 11271998
---

# Design

This is a design documentation for Assignment 2's programming problems.

This assignment has 2 parts.

## Network Emulator using UDP

This part of the assignment is to using UDP ports, to emulator a transmission
over a **lossy** channel. 

The point using UDP has nothing to do with its properties in network/transport
layer (connectionless, fast, etc.), but for
1. Availibility in our software stack and relatively straightforward to do
2. Omitted many checkings that *TCP* has done, therefore make it easier to
fiddle with.

A graphical representation for the connection model:

```
    ==>       ==>
B         A         C
    <==       <==
```

So B and C are 2 parties trying to talk to each other, and A would act as 
the lossy physical layer path (channel).

When B sends to C. B's data will be actually send to A first, then A forward
it to C.

Similar but reverse for C sends to B

### Client (B/C)

The client (B/C) does the simplest job.
Send message out (to A), and receive the message (from A)

For both parties. Only 1 send-to destination, and only 1 recv-from source,
which are both, A.

So it's easy to implement using 2 threads / multiprocesses / select() / poll().

I chose to using 2 threads:
- **S** Reading for message to send (waiting for input)
- **R** Printing message that has received (waiting for network)

(Each of them sharing similarities in structures, it might be feasible to 
futher extract those similarities into abstractions, but not sure if it's a
good idea, or will become ugly just like Java...)

#### Thread S

Thread S will be in charge of sending messages into network.

pseudocode as follows:

```c
while (not done){
  
  res = waitingForIO(input, &msg);  // this is blocking step
  res = addingHeaders(&msg); // <- takes the 
  res = sendToHostByNetwork(hostAddr, msg);
  
}
```

The blocking part is waiting for input to come in.
Once the input is ready, it will send out to host.

If there is anything else to do, then that is in between 2 lines, 
we need to put a "DataLink Layer header" which contains an identifier of
whois the sender. (it makes channel server, A, much easier.)

#### Thread R

Thread R will be in charge of receiving messages from network

pseudocode as follows:

```c
while(not done){
    res = waitingFromNetworkRecv(hostAddr, &msg); // this is blocking step
    res = takesHeaderOut(&msg);
    res = writeOutToIO(output, msg);
}
```

The blocking part is waiting for the data from network to come in.
Once the message is ready, it will write to output (I/O)

Due to the similar structure and pattern, I might actually have a 
function that wraps around the two functions. I'll see... Looks fun...

The host is always A......

