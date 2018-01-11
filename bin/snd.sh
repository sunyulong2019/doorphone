#!/bin/sh

if [ ! -e /dev/snd ]
then
	mkdir /dev/snd
fi

[ ! -e /dev/snd/controlC0 ] && mknod /dev/snd/controlC0 c 116 0
[ ! -e /dev/snd/controlC1 ] && mknod /dev/snd/controlC1 c 116 32

[ ! -e /dev/snd/pcmC0D0p ] && mknod /dev/snd/pcmC0D0p c 116 16
[ ! -e /dev/snd/pcmC0D0c ] && mknod /dev/snd/pcmC0D0c c 116 24

[ ! -e /dev/snd/timer ] && mknod /dev/snd/timer c 116 33

[ ! -e /dev/snd/mixer ] && mknod /dev/snd/mixer c 14 0
[ ! -e /dev/snd/dsp   ] && mknod /dev/snd/dsp   c 14 3
