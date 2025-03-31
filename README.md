# BadHTAB

This is a **hardware-software** based hypervisor (lv1) exploit for Sony Playstation 3. Initially invented by geohot for linux, If you pull certain RAM signal to ground for a short time, write may be skipped.
If you do it while the hypervisor is invalidating a HTAB entry, it may stay valid. Giving us a full read-write permission of a small region of memory at certain location on memory. This location may be later used by the hypervisor itself, allow us to manipulate it and then later gain full access to all of memory.

This exploit has been now ported to GameOS environment, working on every models with PS3HEN. Allows full hypervisor access for Non-CFW consoles for the first time ever. 

This exploit contain two major components:
  * **BadHTAB** - Software side of the exploit, released as a pkg files running on the PS3
  * **ps3pulldown2** - Hardware side of the exploit, Raspberry Pi Pico (RP2040) based. Communicate with PS3 through USB port

Unlike original linux version, GameOS have much more smaller glitch window than linux. This means automation is a must to get successful glitch while remain stable.

Even with that, success rate still remain low **(5-10%)**. This means this is not for daily driver, it is for user with **skill and patience** only.

This exploit **requires soldering**, soldering isn't difficult part. Getting it to boot and stable after solder is.

This exploit are based from xorloser's implementation called **Xorhack**.

