# BadHTAB

This is a **hardware-software** based hypervisor (lv1) exploit for Sony Playstation 3. Initially invented by geohot for linux. If you pull certain RAM signal to ground for a short time, write may be skipped.
If you do it while the hypervisor is invalidating a HTAB entry, it may stay valid. Giving us a full read-write permission of a small region of memory at certain location on memory. This location may be later used by the hypervisor itself, allow us to manipulate it and then later gain full access to all of memory.

This exploit has been now ported to GameOS environment, working on every models with PS3HEN. Allows full hypervisor access to non-CFW consoles for the first time ever. Gaining some of CFW-only features on non-CFW consoles.

This exploit contain two major components:
  * **BadHTAB** - Software side of the exploit. released as a pkg files running on the PS3
  * **ps3pulldown2** - Hardware side of the exploit. Raspberry Pi Pico (RP2040) based. Communicate with PS3 through USB port

Unlike original linux version, GameOS have much more smaller glitch window than linux. This means automation is a must to get successful glitch while remain stable.

Even with that, success rate still remain low **(5-10%)**. This means this is not for daily driver, it is for user with **skill and patience** only.

This exploit **requires soldering**, soldering isn't difficult part. Getting it to boot and stable after solder is.

This exploit is based from xorloser's implementation called **Xorhack**.

This exploit is not persistent and must be run again after reboot.

# Features

After successful run, these thing will be possible:
 * **hvcall 114 everywhere** allow mapping of any memory area without restrictions
 * **New lv1_peek/poke/exec hvcall added** allow lv1 peek(34)/poke(35)/exec(36) through hvcall
 * **Dumping of lv1 memory** dump lv1 memory to file
 * **Boot custom lv2_kernel.fself** allow loading of **ANY** lv2_kernel as long as it is in fself format
 * **Boot OtherOS** allow booting of petitboot bootloader, regain ability to use OtherOS and linux


**Note: If you use boot lv2/OtherOS features, New hvcall will be removed. However hvcall 114 everywhere remains. So you can use that to reinstall it again.**

# Installation (Hardware)

<details>
  <summary> <b> Guides </b> </summary>

<p>

<b>Requirements:</b>
  - Raspberry Pi Pico (RP2040)
  - 0.1mm magnet wire
  - Soldering tools

This guide will focused on superslim only.

![badhtab-npx-001-solder-points](https://github.com/user-attachments/assets/81e5342c-7167-4017-ae92-1010221dfdbe)

These resistor can be found in following ways
 - Service manual
 - Desolder the ram then trace it manually

1. Solder one wire to <b>RQ</b> resistor of each side. <b>Example:</b> first wire into <b>RQ8</b> pin of left side, then second wire into <b>RQ7</b> pin of right side
2. Solder other side of the wire into <b>GP15/16</b> (bottommost) of pico: <b>Example:</b> first wire into <b>GP15</b>, then second wire into <b>GP16</b>

</p>
</details>
