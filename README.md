## Introduction

Toyip is a toy TCP/IP stack running on Linux. Due to my laziness, it only handles protocols above link layer. All the link layer jobs, including interface handling and ARP, are handled by Linux raw socket API. In a word, it's a user-space TCP/IP stack.

## Motivation

Several months ago I started to study LWIP, an opensource lightweight TCP/IP stack for embedded systems. Since I don't have a suitable development board for those embedded operating systems who have LWIP ported to, I have no chance to compile it and see the code runs. Then I came across an idea, why don't I just implement a simple one to see how TCP/IP goes. There I have this toy project, a user space protocol stack just for fun.

## Plan

- Protocols: ICMP UDP TCP
- NAT: A basic one ICMP and UDP supported.
