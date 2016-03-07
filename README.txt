Open Source Line Echo Canceller (OSLEC)
=======================================

[[introduction]]
Introduction
------------

Oslec is an open source high performance line echo canceller.  When
used with Asterisk it works well on lines where the built-in Zaptel
echo canceller fails.  No tweaks like rxgain/txgain or fxotrain are
required.  Oslec is supplied as GPL licensed C source code and is free
as in speech.

Oslec partially complies with the G168 standard and runs in real time
on x86 and Blackfin platforms.  Patches exist to allow any Zaptel
compatible hardware to use Oslec.  It has been successfully tested on
hardware ranging from single port X100P FXO cards to dual E1 systems.
Hardware tested includes TDM400, X100P, Sangoma A104, Rhino E1 etc.
It also works well on the link:ip04.html[IP04 embedded Asterisk
platform].

There is also a project underway to use
http://peter.schlaile.de/mISDN/[OSLEC with mISDN].

Oslec is included in many distributions, including Debian, Gentoo,
Trixbox, Elastix, and Callweaver.

[[testimonials]]
Testimonials
------------

So how good is Oslec?  

+ The http://www.trixbox[Trixbox] community have been testing
Oslec, here is a
http://www.trixbox.org/forums/trixbox-forums/open-discussion/need-people-echo-problems[Trixbox
forum thread] that has feedback from many people.  Start about half
way down the page, or search on the string "night and day" :-)

+ The http://callweaver.org/wiki/Zaptel[CallWeaver] project use and recommend Oslec.

+ Here is some feedback from Oslec users:

[Gordon Henderson, posted in the  Asterisk-user mailing list July 21 2008]

________________________________________________________________________
I switched to OSLEC after testing HPEC on TDM400 boards, and found that it 
worked much better and wasn't limited to the restricted mechanism Digium 
uses for licensing (unlikely as it sounds, I have some clients who do not 
have a connection to the public Internet, and never will for their phone 
system)

It also passes the wife test which HPEC didn't.

It's also free (OS as in Open Source), which HPEC isn't, although that 
wasn't my primary reason for using it - ease of use and "workability" was.
_____________________________________________________________________

[Michael Gernoth, Author of the Zaptel MG2 echo canceller]

________________________________________________________________________
Thanks for OSLEC, it's so much better than my hacked KB1 which is called
MG2 :-)
_____________________________________________________________________

[David Gottschalk]

________________________________________________________________________
Once I managed to get OSLEC installed (it was a big job for me, because I was
missing many of the dependancies and knowledge), it immediately fixed the echo
problems I had been unable to get rid of to date despite lots and lots of
tweeks, tests and fiddling.  Even more suprisingly, it appears to converge
almost immediately from my initial test.

This code is the best since thing since, well, Asterisk !!!
_____________________________________________________________________

[Nic Bellamy, Head Of Engineering; Vadacom Ltd]

________________________________________________________________________
Short version: KB1 < MG2 < OSLEC < HPEC

In the open source line echo canceller category, OSLEC is so far the 
best I've seen.

It converges reasonably fast, and thanks to it's dual-path approach, 
will converge, albeit a bit slowly, on nasty echo paths where cancellers 
like KB1 and MG2 will fail entirely.

The nonlinear processor and comfort noise generator provide for a far 
more pleasant listening experience than KB1/MG2.

It's not quite up to the grade of ADT's G.168 canceller used in Digiums 
HPEC product in terms of convergence speed or tail coverage.
_____________________________________________________________________

[Pawel Pastuszak, Astfin.org]

________________________________________________________________________
Since I installed the first version of OSLEC I was hooked.... It's
been several months now since the alpha version was released; after
using OSLEC Mark and I agreed that we had to have OSLEC as the main
echo cancellation in the Astfin distribution.
________________________________________________________________________
                                                                             
[Pavel Selivano, Parabel Ltd]

________________________________________________________________________
I've been working with Oslec since March of 2007.  Oslec has had some
problems, but, thanks to David, they are solved.  Now, I can say Oslec
is time-tested.  The best, I can note - it is still under development
(with feed-back).  Thank you David for your good deed.

BTW: comparison to Intel IPP's echo canceler have shown, Oslec is very
good :-)
________________________________________________________________________


[[news]]
Oslec News
----------

+ Saturday, June 7, 2008: We have worked out how to use <<rhino,Oslec with
Rhino>> PRI cards.  Thanks Juan Manuel Coronado!

+ Friday, May 16, 2008: Oslec Road Map.  I have kicked off a
discussion of the
http://sourceforge.net/mailarchive/forum.php?thread_name=1210895745.7127.60.camel%40localhost&forum_name=freetel-oslec[development
roadmap] for Oslec.  Is hardware echo cancellation headed for
extinction?

+ Friday, May 16, 2008: Oslec chosen as the **default**
http://sourceforge.net/mailarchive/forum.php?thread_name=20080515131239.GJ27523%40xorcom.com&forum_name=freetel-oslec[echo
canceler for Debian].  Thanks Tzafrir for your work on packaging Oslec
for Debian!

+ Tuesday, May 13, 2008: link:/blog/?p=64[Oslec versus the SPA 3000]
ATA.  A blog post comparing the SPA 3000 echo cancellation to Oslec on
a problem FXO line.

+ Wednesday, February 11, 2008: Oslec mailing list created.  Due to
popular demand (well, Tzafrir mainly!) there is now an
https://lists.sourceforge.net/lists/listinfo/freetel-oslec[Oslec
mailing list].

+ Sunday, February 10, 2008: Zaptel 1.4.8 patch checked into SVN,
thanks Tzafrir.

[[install]]
Installing  OSLEC with Asterisk/Zaptel
--------------------------------------

Notes:

+ This process also installs a system for sampling echo signals that
  is helpful for developing oslec.

+ I assume Asterisk is already installed and tested.

+ I assume you are running Linux 2.6 and Zaptel 1.2.13 or a later
  1.2/1.4 version of Zaptel.

+ I assume you are using a Digium TDM400 line interface card.  Change
  the "insmod wctdm.ko" line below to match your line interface
  hardware (e.g. wcfxo for a X100P).

+ For Zaptel 1.4.x replace the 1.2.13 text below 1.4.x (tar ball and
  patch file).

+ For Linux 2.4 replace the "insmod moduleXYZ.ko" lines below with
  "insmod moduleXYZ.o".

1/ Download, build and install oslec:

  $ cd ~
  $ wget http://www.rowetel.com/ucasterisk/downloads/oslec-0.1.tar.gz
  $ tar xvzf oslec-0.1.tar.gz
  $ cd oslec-0.1
  $ make
  $ insmod kernel/oslec.ko 

Optional: If you want the latest and greatest, replace the "wget" step
with:

  $ svn co http://svn.astfin.org/software/oslec/trunk/ oslec

NOTE: There are patches for Zaptel 1.2.13, 1.2.18, 1.2.24, 1.4.1,
1.4.3, 1.4.4, 1.4.7.1, 1.4.8, 1.4.9.2 and 1.4.11.  It's quite easy to
port to other versions, please feel free to send me a new patch should
you get Oslec working with another Zaptel version.  If you can't see a
patch for your Zaptel version try
http://svn.astfin.org/software/oslec/trunk/kernel[Oslec SVN] for the
latest patches.

2/ Build, patch and install Zaptel.  First obtain zaptel-1.2.13.tar.gz and:

  $ tar xvzf zaptel-1.2.13.tar.gz
  $ cd zaptel-1.2.13
  $ ./configure
  $ patch < ../oslec-0.1/kernel/zaptel-1.2.13.patch    (see note 1 below)
  $ cp ../oslec-0.1/kernel/dir/Module.symvers .        (see note 2 below)
  $ make
  $ insmod zaptel.ko
  $ insmod wctdm.ko 
  $ ./ztcfg
  $ asterisk

NOTE: 1: Use -p1 option for zaptel-1.4.9.2 and above

NOTE: 2: The "cp Module.symvers" step above is optional on many systems;
it stops warnings like WARNING: "oslec_echo_can_create" [zaptel.ko]
undefined!". However on some Linux machines zaptel will not compile
without this step.

3/ These options in zapata.conf are important:

---------------------------------------------------------------------
    echocancel=yes
    echocancelwhenbridged=no
    ;echotraining=400
---------------------------------------------------------------------

The *echocancelwhenbridged=no* allows faxes to pass from FXS to FXO
ports without interference from the echo canceller.  This option is
important for fax signals.

Make sure *echotraining* is disabled when using Oslec - this is not
supported and if enabled will cause the channel to be silent (i.e. no
audio will pass through).

4/ The settings above have been shown to reliably cancel echo in 95%
of cases.  If you can still hear echo you may have one of the rare
cases where your echo is longer than 16ms.  To configure Oslec with a
32ms tail:

---------------------------------------------------------------------
    echocancel=256
    echocancelwhenbridged=no
    ;echotraining=400
---------------------------------------------------------------------

Then restart Asterisk.

[[pbxinaflash]]
PBX in a Flash Install
----------------------

Here are the
http://pbxinaflash.com/forum/showthread.php?t=100&page=3[Oslec install
procedure] for PBX in a Flash. Check the 7-24-08 post in this thread
by Alex728 for Zaptel-1.4.11 instructions.  Thanks JD Austin and Alex.

Matt Keys has suggested that the PBX in a Flash install procedure also
worked well for Ubuntu server.
 
[[rhino]]
Rhino PRI cards
---------------

The rhino-2.2.6 driver for Rhino PRI cards has software echo
cancellation disabled by default.  This means there is no echo
cancellation unless you opt for the Rhino hardware echo cancellation
module.  However the driver is easy to hack to enable software echo
cancellation, and hence Oslec.

In the rhino-2.2.6 driver, file r1t1_base.c, r1t1_receiveprep
function, around line 800:

---------------------------------------------------------------------
static void r1t1_receiveprep(struct r1t1 *rh, int nextbuf)
{
/*
    int x;

    for (x=0;x<rh->span.channels;x++) {
        zt_ec_chunk(&rh->chans[x], rh->chans[x].readchunk, rh->ec_chunk2[x]);
        memcpy(rh->ec_chunk2[x],rh->ec_chunk1[x],ZT_CHUNKSIZE);
        memcpy(rh->ec_chunk1[x],rh->chans[x].writechunk,ZT_CHUNKSIZE);
    }
*/
    zt_receive(&rh->span);
}
---------------------------------------------------------------------

**Solution 1:** Simply un-comment the code and rebuild the driver:

---------------------------------------------------------------------
static void r1t1_receiveprep(struct r1t1 *rh, int nextbuf)
{

    int x;

    for (x=0;x<rh->span.channels;x++) {
        zt_ec_chunk(&rh->chans[x], rh->chans[x].readchunk, rh->ec_chunk2[x]);
        memcpy(rh->ec_chunk2[x],rh->ec_chunk1[x],ZT_CHUNKSIZE);
        memcpy(rh->ec_chunk1[x],rh->chans[x].writechunk,ZT_CHUNKSIZE);
    }

    zt_receive(&rh->span);
}
---------------------------------------------------------------------

**Solution 2:** Bob Conklin from Rhino has suggested this fix that
they have in SVN trunk:

---------------------------------------------------------------------
static void r1t1_receiveprep(struct r1t1 *rh, int nextbuf)
{
    if (!rh->dsp_up)
        zt_ec_span(&rh->span);
    zt_receive(&rh->span);
}
---------------------------------------------------------------------

This version automatically falls back to software echo cancellation if
the DSP-based hardware echo cancellation is not present.  Thanks
Bob!

[[status]]
Status
------

Current status is considered *Stable*.  As of 3 January 2008:

+ Thousands of people (estimated) using Oslec on x86 platforms now.
Oslec is stable and works well.  Oslec has been included in many
distributions, including Debian, Gentoo, Trixbox, Elastix, and
Callweaver.

+ Oslec works well on many different types of hardware, from single
port $10 X100P cards to dual E1s.

+ Oslec has been partially optimised for the embedded Blackfin
DSP/RISC CPU and now runs in real time on the link:ip04.html[IP04 Open
Hardware IP-PBX].  Running on several hundred IP04s now.

+ Real time runs well on FXO/FXS lines where the other Zaptel echo
cancellers (including the latest MG2) struggle.  No tweaks like
levels, fxotrain, or even opermode required so far.  Hardware used for
tests was a TDM400 and X100P card.

+ The simulation code passes many but not all of the G168 tests.
Some problems with high level signals near 0dBm0 and some of the G168
echo models.  Passes maybe 90% of the tests attempted so far, however
not all of the tests and/or range of test conditions have been
attempted yet.  The fails are close calls (like a few dB off), not
complete breakdowns of the algorithm.

+ Oslec has been specifically developed to work with low cost line
interface hardware like the X100P.  The X100P has problems with 60Hz
hum and DC offset in the rx signal, which interfere with the echo
canceller algorithm.  However with the addition of some high pass
filters good results with X100P cards have been obtained using Oslec.
Line interfaces based on chips such as those from Silicon Labs
(e.g. the TDM400) have no DC offset or hum which is one reason they
tend to perform better.

+ Oslec has been optimised to deal with specific problems encountered
with soft phone clients.  Due to the high quality microphones and
sound blaster hardware used on PCs, more low frequency energy is
present compared to normal telephone signals.  When this low frequency
energy is sent to a hybrid it can force the hybrid into a non linear
mode.  Echo cancellers assume a linear hybrid so any non-linearities
make the echo canceller fall over.  The solution is to high pass
filter energy sent to the hybrid to ensure excessive low frequency
energy is removed.  This means inserting a HP filter in the tx path.

[[directories]]
Directories
-----------

The http://svn.astfin.org/software/oslec/trunk/[Oslec SVN repository]
combines several projects (mainly spandsp) used to develop, test, and
run oslec.

spandsp : A subset of spandsp to support testing and development of
	  Oslec.  The echo canceller and G168 test suite source code
	  live in here.

user....: User mode apps, e.g. sampling of echo signals and a unit
          test for measuring the execution speed of Oslec.

kernel..: Builds Oslec into a kernel module.  Also contains patches for
	  Zaptel to allow the use of Oslec.

[[diagnostics]]
Run Time Information
--------------------

Set up a call that uses an analog port, then check out /proc/oslec for
real-time stats:

  [root@homework kernel]# cat /proc/oslec/info
  channels....: 1
  mode........: [13] |ADAPTION|NLP|CNG|
  Ltx.........: 0
  Lrx.........: 211
  Lclean......: 211
  Lclean_bg...: 211
  shift.......: 0
  Double Talk.: 1
  MIPs (last)....: 1
  MIPs (worst)...: 7
  MIPs (avergage): 1

You can turn the various mode switches off and on, for example:

   echo 9 > /proc/oslec/mode

turns off comfort noise, but keeps ADAPT and NLP on.  The mode
switches are listed in the
http://svn.astfin.org/software/oslec/trunk/spandsp-0.0.3/src/spandsp/echo.h[echo.h]
header file (#define ECHO_CAN_USE_*).  The /proc interface only
monitors the first Zaptel call you bring up, see
http://svn.astfin.org/software/oslec/trunk/kernel/oslec_wrap.c[oslec_wrap.c]
for more information.
   
There is a GUI for run-time control of Oslec, called the Oslec Control
Panel.  For example you can Enable and Disable the echo canceller in
real time, as you are speaking.  If you Reset the echo canceller you
can hear how long it takes to converge again.

NOTE: The command line tool *dialog* must be installed on your system
to use the Oslec Control Panel.

  $ cd kernel
  $ ./oslec-ctrl-panel.sh 

image::/images/echo/oslec_control_panel.png[Oslec Control Panel]

[[speed]]
Execution Speed and Optimisation
--------------------------------

In the user directory there is a
http://svn.astfin.org/software/oslec/trunk/user/speedtest.c[speedtest.c]
program that measures how many MIPs the echo canceller consumes and
estimates how many cancellers can run in real time.  The current x86
code is vanilla C and could be greatly improved with MMX or SSE
optimisations, some of which are already coded in spandsp.  There is
also much that can be done to improve execution speed on the Blackfin.

Steve's spandsp code includes some support for MMX and SSE2
optimisation.  Define USE_MMX or USE_SSE2 in the Makefile to use this
option.  At this stage only the fir.h filter code is optimised, oslec
would run much faster if lms_adapt_bg() in echo.c was also ported to
MMX/SSE2.  With USE_MMX defined, the speedtest.c program dropped from
33 MIPs to 20 MIPs per channel for 256 taps/32ms.  If using MMX in the
kernel (e.g. with zaptel) make sure you compile zaptel with
CONFIG_ZAPTEL_MMX to ensure the FPU state is saved in the right
places.  

To enable MMX/SSE modify this line of oslec/kernel/Makefile:

----------------------------------------------

all::
        $(MAKE) -C $(KDIR) EXTRA_CFLAGS='$(KINCLUDE) \
        -DUSE_MMX -DUSE_SSE2 -DEXPORT_SYMTAB -O6' \
        SUBDIRS=$(PWD) modules

----------------------------------------------

Thanks Nic Bellamy for help in testing MMX and Ming-Ching Tiew for
providing the correct command line for compiling with MMX/SSE.

Some notes on further optimisation:

1/ How do lots of MIPs in ISR affect total system performance?  For
example if we are using 25% of MIP in ISR does * still run OK?  Find the
CPU load knee different for user versus kernel mode cycles
consumption.

2/ Estimated Oslec MIPs are 5(N)(Fs), N is the filter size (number of
taps), Fs=8000 is the sampling rate.  Factor of 5 is comprised of 1
for each FIR filter (forgeround and background), 2 for LMS, 1 for overhead.
This suggests that when optimised around 10 MIPs for a 32ms tail.

3/ There are some more notes on optimisation for the Blackfin in
http://svn.astfin.org/software/oslec/trunk/spandsp-0.0.3/src/echo.c[echo.c].

[[credits]]
Background and Credits
----------------------

Oslec started life as a prototype echo canceller and G168 test
framework from Steve Underwood's http://www.soft-switch.org/[Spandsp]
library.  Steve wrote much of the DSP code used in Asterisk, and the
Zaptel echo cancellation code is heavily based on his work.

Using the Spandsp G168 test framework, a high performance echo
canceller has been developed and carefully tested.  Working together
with alpha testers the performance was brought to a beta state in June
2005.  <<links,More>>.  Since then many thousands of people have
installed Oslec.

Thanks to Steve Underwood, Jean-Marc Valin, and Ramakrishnan
Muthukrishnan for their suggestions and email discussions.  Thanks
also to those people who collected echo samples for me such as Mark,
Pawel, and Pavel.  Thanks Nic Bellamy for help with testing,
explanation of long path issues, and MMX support.  Thanks Tzafrir for
testing and patches/enhancements and Dmitry for help with
multithreaded and locking issues.

Thanks Vieri for finding a circuit with a tail > 32ms, and Patrick for
submitting a Zaptel 1.2.18 patch.  Thanks Dave Fullerton and Carlton
O'Riley for Zaptel 1.4.3/1.4.4 patches.  Thanks Bill Salibrici for
finding a memory leak. Thanks Michael E. Kromer for submitting a
Zaptel 1.4.7.1 patch, and Tzafrir for the Zaptel 1.4.8 patch.  Thanks
Russ Price for the Zaptel 1.2.24 patch.  Thanks Chris Notley for the
1.4.11 patch.

Thanks to Peter Schlaile for porting Oslec to mISDN.  Thanks also to
Kristijan Vrban for sending me some ISDN hardware so I can help with
the ISDN/Oslec testing!

Thanks to Tzafrir and Faidon for helping debug the muted audio
problems on 64 bit systems.

Thanks Rudolf E. Steiner for testing Oslec on SMP and multiple E1
systems.

[[sample]]
Sampling Zaptel Echo
--------------------

Introduction
~~~~~~~~~~~~

I have developed a system for sampling echo on running Zaptel/Asterisk
systems.  If you do experience any echo with Oslec, please use this
system to sample the echo, then email the sample files to me.  

See the http://www.rowetel.com/blog/?p=18[Part 1 blog post] for more
information on sampling echo.

The nice thing about is that it doesn't interfere with your running
Asterisk system.  If you hear echo at any time you can fire up a
console and run "sample" to capture real-time data from the Zaptel
port.

There are other ways to capture audio from a running Asterisk system,
for example using the built in Asterisk play and record applications.
However this system is designed to capture *exactly* the signals being
fed to & from the echo canceller - preserving the exact timing of the
signals and with no intermediate buffering.  This is very important
for echo cancellation work - the algorithms depend on an exact timing
relationship between the transmit and receive signals.

The system simultaneously captures the receive signal both before
*and* after the echo canceller - something that is difficult to do
with the built in Asterisk functions due to the location of the echo
canceller deep in the Zaptel driver.

If you would like to help further develop Oslec, please
mailto:david_at_rowetel_dot_com[send me] your echo samples! I would
welcome any samples of your echo signals, for example where the echo
canceller isn't working well, and also cases where it does work
well. By comparing the two cases we can learn a lot about the
strengths and weaknesses of the algorithm.

Installing
~~~~~~~~~~

1/ Install Oslec ("HowTo - Run OSLEC with Asterisk/Zaptel" section above).

2/ If you would like to use zaptap without oslec change the selected
   echo canceller in zconfig.c and rebuild/install zaptel.

3/ Compile sample.c:

     # cd user
     # make

4/ Create a device node.  I used major number 33 as it was
   free on my PC:

     # mknod -m 666 /dev/sample c 33 0

   If 33 is not free choose a free major number and change the
   #define SAMPLE_MAJOR in zaptel.c and recompile.

Note it is important to "insmod zaptel.o" from inside the zaptel
directory and don't forget the ".o" (.ko for Linux 2.6).  Otherwise
insmod will use the previously installed version of zaptel and
"sample" won't work.

5/ Make the call to the Zaptel port you wish to sample.  Run
   sample while talking:

     # ./sample test 1 5

this will create test_tx.raw, test_rx.raw & test_ec.raw. There
will be a few messages on the console as the driver does it's
thing, you can check these with dmesg.

6/ You can check your samples by playing them back through your
   sound card, for example:

     # play -f s -r 8000 -s w test_ec.raw

7/ To convert the raw files to wave files (this is more convenient for playing
   and processing with the Oslec simulation):

     # sox -t raw -r 8000 -s -w -c 1 test_ec.raw test_ec.wav
     # play test_ec.wav

8/ There is an Octave script pl.m to help plot the samples:
    
     # cp /your/test/samples ~/oslec/user
     # cd ~/oslec/user
     # octave
     octave:1> pl("test")

Configuration
~~~~~~~~~~~~~

+ Zaptel/Asterisk 1.2.13, 1.4.0 or 1.4.1.  However other versions should work
  OK.
+ Linux 2.4 or 2.6 Kernel
+ Digium TDM400 hardware
+ My zapata.conf for the FXO port under test is something like:

---------------------------------------------------------------------
    signalling=fxs_ks
    echocancel=yes
    ;echocancelwhenbridged=yes
    ;echotraining=400
    group=2;
    conext=incoming
    channel =>4
---------------------------------------------------------------------
 
Note echo training is switched off, although this didn't make much
difference to the FXO Port when using the built-in Zaptel echo
cancellers. (helped the the FXS port though).  Oslec does not support
echo training.

[[support]]
Support
-------

https://lists.sourceforge.net/lists/listinfo/freetel-oslec[Oslec
mailing list].

[[further]]
Further Work
------------

Here are some ideas for further work.

1/ Make info screen on control panel update each second automatically.

2/ When e/c is reset during a call it converges faster than at start of
   calls.  This suggests e/c is wandering off into dumb states before
   the call is connected.  Perhaps disabling adaption until the call
   is connected would help with faster convergence

3/ Preserving the state of the e/c between calls is also a very good
   idea, current zaptel design destroys the e/c at the end of a call.

4/ Switch on and test SSE and MMX filter code for x86.  Code LMS
   update for x86 in SSE or MMX.
   
5/ Add a feature to /proc/oslec to extract the current estimated
   impulse response.  Use gnuplot or Octave to plot it in real time.

6/ Attempt to speed convergence.

7/ Set up test scripts for greater coverage of G168 tests.

8/ If necessary develop a sparse approach to handle 128ms tails.

[[Simulation]]
HowTo - OSLEC G168 Simulation
-----------------------------

The simulation form of Oslec is useful for Oslec development.  It is
much easier to develop using a non-real time, user mode program than
saying 'Hello,...1,...2,....3" down a telephone line.  The simulation
dumps internal states while it is running, which can be plotted and
analysed using Octave.

1. Read spandsp-0.0.3/README and make sure you have installed the
dependencies like libtiff-devel, libaudiofile-devel, fftw-devel.

2. Make, build, run a test:

  $ cd spandsp-0.0.3; 
  $ ./configure --enable-tests
  $ make
  $ cd tests
  $ ./echo_tests 2a

3. You can change the echo path model with -m [1..7] (default 1)

4. You can change the ERL with -erl [0..whatever] (default 10.0)

5. You can plot the internal states using Octave:

  $ cd spandsp-0.0.3/echo_tests
  $ octave
  octave:1> echo_dump

  The st= and en= statements at the top of echo_dump.m control which
  part of the waveform is plotted.

6. There are some sequences of tests set up in the script files:

  $ ./g168_quick.sh (useful for a quick sanity test)
  $ ./g168_tests.sh (more comprehensive set of tests)

7. For more options:

  $ ./echo_tests -h

8. Counting passes (say if u want to see how many tests pass after making
   a change):

  $ ./g168_tests.sh > lms16bit.txt
  $ cat lms16bit.txt | grep PASS | wc -l

[[links]]
Further Reading
---------------

+ http://svn.astfin.org/software/oslec/trunk/spandsp-0.0.3/src/echo.c[echo.c]
is the heart of the Oslec echo canceller.

There are several blog posts documenting the development of Oslec:

+ http://www.rowetel.com/blog/?p=18[Part 1 - Introduction] discusses the myth
of hardware echo cancellation and the concept of an echo canceller
developed using echo samples collected by a community.

+ http://www.rowetel.com/blog/?p=21[Part 2 - How Echo Cancellers Work]
is an easy to read introduction to echo cancellation for C programmers.

+ http://www.rowetel.com/blog/?p=22[Part 3 - Two Prototypes] discusses
two algorithms that were developed as candidates for Oslec.  This is a
fairly "hard core" DSP post - some familiarity with echo cancellers is
assumed.

+ http://www.rowetel.com/blog/?p=23[Part 4 - First Phone calls] talks
about the very first real-world phone calls made using Oslec.

+ http://www.rowetel.com/blog/?p=33[Part 5 - Ready for Beta Testing].
Walks through some of the alpha testing bugs and how they were fixed,
discusses open development methods and the need for 128ms tails.

Some useful links:

+ Ochiai, Areseki, and Ogihara, "Echo Canceller with Two Echo Path
Models", IEEE Transactions on communications, COM-25, No. 6, June 1977. 
http://www.rowetel.com/images/echo/dual_path_paper.pdf[download].

+ The classic, very useful paper that tells you how to actually build
a real world echo canceller: Messerschmitt, Hedberg, Cole, Haoui,
Winship, "Digital Voice Echo Canceller with a
TMS320020. http://www.rowetel.com/images/echo/spra129.pdf[download].

+ A nice
http://en.wikipedia.org/wiki/Least_mean_squares_filter[introduction to
LMS filters].

+ Good introduction to http://www.cisco.com/univercd/cc/td/doc/cisintwk/intsolns/voipsol/ea_isd.htm[echo on VOIP calls] from Cisco.

