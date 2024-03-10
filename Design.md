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

### Server/Router (A)

rather than being an actual server, (from application pov sense)
the job of A is to simulate a channel/message-passing-medium which would drop 
the messages with a specified probility.

Again, the reason to use UDP is to just by-passing the 

Since A would also need to simulate a situation of the propagation delay.

To make things more structurally sound and modular, I will use two threads. 
To enable the communication between the two endpoints, a shared queue will be 
required for upholding the messages.

The queue will be implemented with a linked list type LIST and library 
procedures defined by `listmin.h`. (borrowed from John Miller, removed the ones
that is not required for the scope of this project.)

Note that the list is not thread safe, 
so need to create a pthread_mutex to protect the list.

The list should be both accessible by the sender and receiver.
To ensure safety and easier debug, I'll make the list allocated
on main's stack. Since the threads will be sharing the same memory space,
both threads can accesses the list if the list address is passed to them.

note that different from the endpoint implementation, which keeps sending and 
receiving, as we have a delay parameter in the server, the message might be 
filled up fairly quickly... 

In the first version, I'll simply malloc in the receiver, put the address on Q,
let the sender thread send then `free`. 

In a better version, I'll maintain a linked list that keeps buffering list of
messages. (2 lists: - freeList, - QList)

(JUST REALIZED...)
BAD DESIGN IDEA....
should do the binding inside main, then pass the sockets to the threads.
Threads only do `sendto` and `recvfrom`...

ahhh could have saved hours of debugging here...

#### Channel: Receiver

This thread is the one that is in charge of receiving the incoming messaages 
and adding the message to the end of the queue.

It will be blocked by the RECV primitive. When something arrives from network,
the RECV would return, and this thread will add the message in the shared Q.

After this, it will going back to waiting for another message via RECV.

pseudocode

```c
while (!done) {
  recv_from(PORT, &msg);
  add_to_queue(msgQ, msg);
}
```

The Receiver should be as simple as possible so it would spend most time on 
`recv` so not to accidentally loss data.

#### Channel: Sender

This thread is the one that is in charge of taking messages out of the queue,
and sends out to the destination. (this is related to the message structure 
which I will mention a bit later)

Aside from being the channel that is forwarding the message, 
as the specification, the sender will need to simulate the effect of
 1. propagation delay (from input argument `d`)
 2. loss probability (from input argument `p`)

One note is that, being the server, it needs to figure the destination
of the message based on receiving result.

in that case, 

```c
/* in receiver thread */
recvfrom(sockfd, buf, MAX_MSG_SIZE - 1, 0,
            (struct sockaddr *)&their_addr, &addr_len)
```
note that `their_addr` is a type `struct sockaddr_storage *`
can be casted to `struct sockaddr_in *` (ipv4), which, then,
can extract `sin_addr` from the struct. 

```c
struct in_addr senderID = 
  ((struct sockaddr_in *) their_addr) -> sin_addr;
```

This field can be used to uniquely identify the sender.
Such identifier is also the same after casting and 
extracting from the `p->ai_addr` after walking through the 
linked list of `p` from `getaddrinfo`.

```c
struct in_addr rcvId = 
((struct sockaddr_in *) ((p->ai_addr)->sa_data))->sin_addr;
```

Sice A is only caring about 2 end points,
when transmitting, just find the `rcvId` that is different from the `senderID`.

I'll make some helper functions to wrap such routine.

However, since its listening on 2 different endpoints, and involves some
interaction with the queue.
I'll give up extracting extract the routines as functions, but directly reuse 
the code by copy-pasting.

I'll hold on to the oppurtunity for possible extraction 
(maybe macros can help me?).


UPDATE!!
That did not work out...

`getnameinfo` finds the source correctly...
So, in receiver, I can find `host` and `service` 
from incoming message.

I should be able to cross-examine in sender to match the 
correct host and service.

UPDATE2!!
I am the dumbest piece of 💩！！
Of course you need to tell the whoever you sending to, where to reply!

Why am I assuming that the sending service has ANYTHING to do with the
listening part??!!?! I don't understand myself!!

So now the plan is, make it (kind of TCP way that), upon the connection
(modifying the endpoint modules), the endpoint should send 1st message
to middleend about WHERE to shoot reply at!

For example, for such output:

```
/partA-middleend 0.2 1 4000 localhost:4001 localhost:4002
listener: waiting to recvfrom...
connected to dest 1,    host: 127.0.0.1,        serv: 4001
connected to dest 2,    host: 127.0.0.1,        serv: 4002
[receive_thread]: got message:
host: 127.0.0.1,        serv: 37433
listener: got packet from 127.0.0.1
listener: sender id for this packet is 1
listener: packet is 2 bytes long
listener: packet contains "a
"
[receive_thread]: got message:
host: 127.0.0.1,        serv: 54755
listener: got packet from 127.0.0.1
listener: sender id for this packet is 1
listener: packet is 2 bytes long
listener: packet contains "a
```

Above is a scenario such that each of the 2 end points sending
something through the middleend. 

Therefore, we can make a new rule that, for each of the endpoints,
upon connection, before transmitting any messages, should first send
a message about the service (port) it is listening to.

Middle-end would see this, and `getnameinfo` would give the info
for the host and service it is coming from.

Middle-end should add this to a mapping that, for every other time
it sees a message from such source, it would be able to find the 
corresponding destination.

To make things less complicated, let's don't drop any packet on this
initial stage.


A map(-like) data structure would be very helpful in this situation, 

Turns out we actually need a 3-way table, and we set it up and accessing
it in inverted ways.

| Endpoint 1 | Endpoint 2 |    desc     |
|------------|------------|-------------|
|     H1     |     H2     |  Hostname   |
|    P_r1    |    P_r2    | port-receive|
|    P_s1    |    P_s2    |  port-send  |

For each of the endpoints, it has 3 parts of address:
`Hx`, `P_rx`, `P_sx`.

Initially, main thread (with cmd arguments), should fill in `Hx P_rx` for both
endpoints from the commandline argv. With `P_sx` still left empty.

When the first message come, which, the message itself is `rply_to`, 
which is `P_rx` when this is being read... we can also get the corresponding
`H_x` and `P_sx` via `getnameinfo`, with the 3 information, we can update
the table `(H_x, P_rx) => P_sx`.

For all later messages from receiver, the receiver would put on `(H_x, P_sx)`
along with the message on the queue. 

The sender would map `(H_x, P_sx) => (H_y, P_ry)`. 
Then, (invertly) using `getnameinfo`, find which existing socket is matching
`(H_y, P_ry)`. Finally, send the message.

This is super duper robust (in terms of checks and scalilibility (??really?))
That would
1. Allowing endpoints with same host name or same port name, as long as the 
combination of `(H_x, P_rx)` are different.
2. Allowing port-forwarding. Endpoints could have using a proxy port to receive
(However, it still requires the same hostname to infer the other port).
3. Error checking. Sense in 1st time when the address info is corrupted.

(yeah, Feels like I'm really trying to sell this over complicated thing...)

For easier manipulation, have a function such that can figure out which
sender to use.

```c
struct addrinfo * pickToSend(
        struct addrinfo * p1, 
        struct addrinfo * p2,
        struct sockaddr * fromAddr, 
            socklen_t fromAddrLen, 
        LookupTable * tbl);
```

Based on the `fromAddr`, which is obtained from `recvfrom`, 
choose the right `addrinfo` node to return. This node would
be used when to send the message.

##### Simulating Delay: polling-watchdog

To simulating the delay, I would simply let the server sleep for $2d$ units of
time, then take the message out from a queue. 
If there is no message in the queue, sleep again and check when wake up.

This would be the most efficient (polling-watchdog implementation) 
instead of constantly polling.

the reason for using $2d$ instead of $d$ unit of time is that, the thread will
(most likely) to be sleeping. The thread could have slept for anything between
$0$ and $2d$ unit of time, which, the message would wait for anywhere between 
$2d$ and $0$ unitof time. Ignore the effect of time being quantitized on 
computer's clock, on average,
the message would be delayed for $\frac{2d + 0}{2} = d$ unit of time, which, 
even each individual message might be delayed anywhere between 0 and $2d$, 
on average each message would be delayed for $d$.

This is a reasonable trade-off between the accuracy and design simplicity.

#### Simulating Packet-Lossing: RNG

Simulating packet-lossing would be easy. Just using an RNG.

Get a random number (0~1), if it is between 0 and p, then drop, otherwise, send
the message to dest.

If time allows, I'll use random numbers from digits of pi, in celebrate pi-day.

pseudocode for sender thread:

```c
while(!done){
  no_msg=true;
  while(no_msg){
    sleep(2*d);
    no_msg = (msg=dequeue(Q))==NULL;
  }
  // got a message!
  r = random(0,1)
  if (r > p){
     dest = get_dest(msg);
     msg_content = get_content(msg);
     send(msg, dest);
  }
}
```



### DataLink layer Selective Repeat

now with the UDB channel set, we can use the layer as a simulation of a lossy
physical layer.

Using the endpoints to act as 2 sides in data link layer, implement the 
selective-repeat protocol with a sliding window.

Each side would have a sequence number. I choose the number to be in a range
within 0-255, which, would be fit in a single byte so without need of byte-
order conversion. 

The idea is that. For each packet B sends to C, when C receives, C would have
either send a ACK, or send a NAK when next packet arrives (the order is inferred
by the sequence number).

On the side of B, it keeps a buffer that is like a sliding window.
Whenever hears ACK, and when it is in order, it increases its lower bound,
whenever sends out a packet, it increases its upper bound.

The buffer should be simple to implement with a list library. And for the window
it would just rotate within 255 units. 

I originally thought it is apporachable with the server sending out a WRAP
message at 127, informing the client that it is about to WRAP the number around.

However, due to the natural of such lossy channel, the WRAP or ACK to WRAP might
have lost, and we went into a Bayzantine Generals problem....

#### Number Range 

By wrapping the number, the finite set of integer turns into an ordered sequence
on both sides.

When deciding the ordering, it is based on assumption that the window size
(span) should always be less than 128. Hence, the window would always be on
lesser than half of the number range.

let $k$ be an arbitrary number in the range of 0-255.
$W$ be our window with $|W| < 127$.

Then, for any value $s$ from the window, $k=W_0+|W|$ be the next one after the 
window, the right-distance would always be less than 128.

We only keep track of `right-distance` and hence, we can define the ordering 
in this way:

The right-distance $d$ from $A$ to $B$ is defined as:
    $d$ = $B - A$ if $B > A$
        = $B + 256 - A$ if $B < A$

#### Sliding Window: Sender

The sender would keep a list of buffer for anything that is not yet ACKed.
The buffer would have an upperlimit of 127, and would be forced to wait for
ACK of the one coming, until then the window can begin to move and it can 
keep sending again.

The `ACK_x` comes with a sequence number `x`, telling the sender about
which one has been acked and which one is rejected.

Plan is to use a struct union which is 128 bits (16 bytes long) in total,
and whenever it receives a `NAK`, it would stop sending other data, begin
retransmitting the lost frame until it hears the `ACK` which it can then move
forward.

#### Sliding Window: Receiver

On the receiver's side, it need to interprept the incoming message and sequence
number, also aware of the ordering.

A loss of a packet would not be explictly noticed, but the receiver would able
to infer it from the sequence number.

For example, if receives 2 4 5, which, upon `ACK_4` it would include the `NAK_3`
too. 

The above explaination to the number range would ensure that it would know when
seeing a sequence less than what it expected, which, would propagates to an err.

