# Distributed Group Membership Service

This service maintains, at each machine in the system (at a daemon process), a
list of the other machines that are connected and up. This membership list is a
full membership list, and needs to be updated whenever:

1. A machine (or its daemon) joins the group.
2. A machine (or its daemon) voluntarily leaves the group.
3. A machine (or its daemon) crashes from the group (assuming that the machine does not recover for a long enough time).

Here we are implementing a crash/fail stop model i.e. when a machine rejoins, it must do so by establishing a new id.

To implement this concept, the different incarcenation of same machine are distingished by timestamp. Effectively, a node or machine is uniquely identified by:-

`Node_id := machine_id(given by user) + timestamp + IP_Address`

The protocol followed by the system for implementing the requirements are as follows:-

- Using virtual ring as a backbone or topology.
- Implemented ping-ack style to detect failure similar to SWIM protocol but without random pings to reduce communication complexity.
- Each VM has 3 monitors for tracking its liveliness, to tolerate maximum of 3 simultaneous failures of different machines. This is a configurable parameter which can be changed in config.

## Key Concepts

- Each machine first pings `Introducer` (which is also part of the ring), which then acks back with the latest membership list to the sender.
- Introducer must be live for any new members to join. Failure of Introducer does not affect the current members.
- Each machine monitors its 3 successive neighbors and periodically send pings to ensure the machines are alive.
- Once threshold pings have been dropped by a machine, the monitor declare the machine as dead and sends the updates to its successors, which dessiminate them further to their successors in the ring.
- Once a machine receives the `LEAVE` message for its machine, it voluntarily leaves the ring and must join the ring again.
- It uses UDP instead of TCP as transport layer, as it is faster than TCP, though unreliable, which is taken into account.

### Different Message Types

- `JOIN` - Sent by the new member to the introducer along with its information relevant to join the group.
- `JOIN_ACK` - Sent by the introducer to the sender along with the membership list, this acknowledges the entry of new member to the group.
- `INTRODUCE` - Sent by the introducer to its successors in the ring to introduce the new member joined in the group.
- `PING` - Sent by monitors to the machines they are monitoring at configurable periodic intervals.
- `PING_ACK` - Sent as a response to the incoming ping from monitors to alert their live status.
- `LEAVE` - Sent by the VM who voluntarily wants to leave the group or it can also be sent by monitors to alert their successors that one of the machine it was monitoring has exceeded the time threshold to respond.

### Instructions for running

- Run `make`
- Run `./bin/exec {machine-id} {is_introducer(0 or 1)} {port}`
- Port for introducer should be `3000`.
- Here are the user commands you can execute:-
  - `JOIN` - to join the group
  - `LEAVE` - to leave the group
  - `LIST_SELF` - To print self membership details
  - `LIST_MEM` - List Membership list at the host



