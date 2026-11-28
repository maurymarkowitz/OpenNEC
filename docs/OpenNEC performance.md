OpenNEC performance
===================

Roger Cox, W8IO, publishes a [NEC Benchmark page](https://www.w8io.com/nec-benchmarks.htm) that lists the time to run two large decks using different NEC engines and machines. The first file is a whip antenna with a hand-crafted ground plane with many segments, and the second is a complex log-periodic Yagi. The LogYag is a more typical antenna model, and the relative performance between engines on that file will likely match your own decks.

OpenNEC has added several performance improvements using these decks to guide development, including BLAS/LAPACK support, OpenMP matrix filling and matrix reuse. This makes it among the fastest NEC engines available. To get the best performance will take a little experimentation, see the section on Thread settings below.

Simply put: OpenNEC is *dramatically* faster than older engines, often on the order of 15 to 40 times!

The decks and the benchmark script used to measure them can be found in [`tests/benchmark_tests/`](../tests/benchmark_tests/). See the instructions at the end on how to run these on your platform.

Thread settings
---------------

OpenNEC has modified the original nec2c code to support multi-threaded code. This makes parts of the code run about as many times faster as you have cores in your CPU, which is normally between 4 and 8.

One annoyance is that the performance in NEC is best when the number of threads is equal to the number of physical cores. Unfortunately, there's no standard way to know the number of cores. The code for getting the number differs across platforms, and to add confusion, many CPUs have different *types* of cores which may or may not be returned in the count.

So for now, in order to get OpenNEC to deliver the best peformance for your system, you'll want to look up your machine's specs on the 'net, and find the "performance cores" number - or something like that, the name varies. You can then specify that number of threads when you run the program using command-line variables.

For instance, on a machine with eight performance cores, like the M2 Max, you would want to try:

`OMP_NUM_THREADS=8 VECLIB_MAXIMUM_THREADS=8 ./onec` ...

The only change for other Unix systems and Windows is the name of the second variable:

`OMP_NUM_THREADS=8 OPENBLAS_NUM_THREADS=8 ./onec` ...

You'll want to experiment with the numbers; some machines prefer a larger number than the number of cores, and others strongly prefer smaller, but the majority seem to want threads = cores.

Background
----------

The original idea behind OpenNEC was to make a version of the nec2c code that could be compiled directly into GUI programs. Previous GUI programs like 4nec2 write out a NEC deck, call an external program to run the calculations and write a report file, and then read and parse that file for display. This round-trip introduces delays, which are significant for smaller antenna designs where the time needed to launch the program is often as long as the calculations. By allowing the functionality to be called directly from the GUI, thereby removing the program startup time, OpenNEC could make GUI programs much faster and more interactive.

A number of authors have attacked the performance of the underlying NEC engine itself. A common way is to replace NEC's original hand-crafted Fortran matrix math code with one of a variety of fast multi-threaded math libraries. These are able to run in parallel across multiple cores (sub-processors), which is a feature of all modern computers. NEC's calculations are "embarrassingly parallel", and it speeds up roughly with the number of cores.

By the time the OpenNEC project started, most of these libraries had gone away in favour of using [OpenBLAS](http://www.openmathlib.org/OpenBLAS/). Modifying the original nec2c matrix code to call BLAS turned out to be relatively simple, and was included in the very first versions of OpenNEC for macOS and Unix. This resulted in a major boost in performance, on the order of 3 times for most files, and as much as 7 times for very large decks on an 8-core machine - almost linear. Similar code for Windows was added starting with the 2.2 releases.

Steven Burns, KK7KNB, has been modifying the nec2++ code with similar goals, as part of the [momwire](https://github.com/stevenmburns/momwire) and [AntennaKNoBs](https://antennaknobs.dev) projects. In addition to BLAS, this code added OpenMP support to allow a particular operation, the matrix fill, to also run in parallel. This greatly speeds the setup of complex designs, before BLAS is let loose. He ported these changes back to OpenNEC, and with some further tweaks to make the C gods happy, nec2++ and OpenNEC now offer similar performance on most decks.

One thing you have to be careful about when doing these sorts of changes is that all of these bits of code have their own overhead that can actually slow down the performance on smaller decks. So if you concentrate on speeding up the big decks you might be making the smaller real-world decks people are using go slower. So all of this has to be benchmarked at every step.

Baseline performance
--------------------

To provide a performance baseline, I attempted to run these decks with the original Fortran code from the [nec2d-SX source code](https://www.qsl.net/4nec2/supfiles.htm) found on the 4nec2 web pages, with the segment count increased to 11k and the in-deck comments stripped out. However, neither deck would run, reporting `SEGMENT CONNECTION ERROR...`. This turned out to be due to a fixed limit on the number of connections between segments, 60, which both of these files had issues with at one point or another.

[nec2c](https://github.com/KJ7LNW/nec2c) is a relatively direct port of the original NEC-2 Fortran code. A key improvement is that it removed various fixed-memory-size objects and made them dynamically grow with the model size - the segment connection limit is one of the things this fixed. Its times are very similar to the same decks run under nec2d, so it makes a reasonable alternative to get some baseline numbers. Here is the latest nec2c version 1.3 running these tests on an M2 Max.

| Deck   | Seconds |
|--------|--------:|
| LogYag | 64.68   |
| Whip   | 396.52  |

These numbers are used as "performance value 1.0" and the "Times faster" columns below are the number of times faster than these values. Larger values are better - 10 means ten times as fast, 5 means only five times as fast.

Previous best
-------------

Cox's page lists a number of different engines and platforms. The fastest engine at the time of the last update, in June 2023 when this is being written, was Athan Papadimitriou's [NEC2/MP 1.5](https://web.archive.org/web/20130502013310/http://users.otenet.gr/~jmsp/) being run on a Core i9-12900H machine with six performance cores and another six efficiency cores.


| Deck   | Seconds | Times faster |
|--------|--------:|-------------:|
| LogYag | 38      | 1.7          |
| Whip   | 67.8    | 5.8          |

These results show the improvement you get from using the BLAS code on a machine with six performance cores. Whip, in particular, scales almost perfectly. LogYag is more complex and doesn't do quite so well.

These numbers are not *directly* compariable to the nec2c results above, as it's a different machine. Unfortunately, the source code was not published, and the only binary releases were on the PC, so we have to fudge it. GeekBench states the M2 Max is about 12 to 25% faster, so if we could run on the M2 we would expect to see around 8 times faster for whip, and 2 for LogYag.

NEC2++ results
--------------

With the exception of adding BLAS, many of the performance upgrades to OpenNEC were originally part of momwire's nec2++ modifications. The following measurements are the fastest of five runs using the nec2++ code built on the M2 with OpenBLAS, LAPACK and OpenMP, and `OMP_NUM_THREADS=8` and `OPENBLAS_NUM_THREADS=8`, suitable for the testing machine. The whip was run from a temporary copy with apostrophe comments removed, and the wire-intersection check was disabled because it rejects the mesh.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| LogYag | 9.34    | 6.9   |
| Whip   | 17.48   | 22.7  |

This shows the further improvement that comes with the parallelization of the matrix setup, which NEC/MP lacked. This gives another three times boost.

Another upgrade in the nec2++ code is an optional system that examines the geometry and can re-used cached values instead of recalculating them all the time when building up the inputs to the BLAS calculation. This is known as "fill reuse". The effect depends on the layout of the antenna, and doesn't always improve things. The following times are with that feature turned on.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| LogYag | 9.75    | 6.6   |
| Whip   | 12.91   | 30.7  |

The matrix that gets fed into BLAS contains a list of every segment in the design, and how it interacts with the others. So for whip, which has 4,392 segments, it ends up creating a matrix describing 19.3 million interactions. The reuse concept writes down a little "key" for each of these as it is building them, and then looks to see if many of them share the same key. If enough do, there's no need to build the entry for that slot from scratch, it can just copy it from an earlier matrix.

The reason this effects the whip so much is because the ground plane has so much similar geometry. The effect of the leftmost upper segment on the plane is ultimately the same magnitude as the rightmost lower one, so we can just flip the signs. It doesn't work so well on LogYag because the elements are different lengths, so there's less symmetry. In this case it falls back to the original fill code. Testing whether we should fall back adds a small amount of overhead. But it's not enough to explain the extra second seen here.

OpenNEC results
---------------

Now let's compare all of these results with OpenNEC. These numbers are from version 2.4.2 built as a production binary using the clang complier, `-O2` optimization, Apple Accelerate (which contains BLAS) and OpenMP. Both decks were run five times with `OMP_NUM_THREADS=8` and `VECLIB_MAXIMUM_THREADS=8`, which matches the 8-core system, and the times below are the fastest of the five.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| LogYag | 4.60    | 14.1  |
| Whip   | 12.35   | 32.1  |

OpenMP is as much as 30 times faster than the nec2c baseline, three or more times faster than the previous record holder NEC/MP, and is about 50% faster than nec2++.

OpenNEC 2.4.2 added the fill reuse feature as well, but also added a twist of its own. There is some overhead in deciding whether or not to attempt reuse, it has to calculate many entries and test them before it can say whether or not to fall back. In OpenNEC, there is an additional test that samples a small sub-set of the matrix, about 20,000 items out of the (often) hundreds of thousands, and if they mostly don't match then it's not going to work and it stops trying before calculating too many points.

With fill reuse and the early-exit test enabled, the fastest times from five runs follow.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| LogYag | 4.62    | 14.0  |
| Whip   | 10.13   | 39.1  |

The small effect of the early exit is visible here, barely, as the LogYag takes only 2/100th longer to run as opposed to as much as 5/10ths of a second without the early exit.

It is curious that OpenNEC outperforms nec2++ by this much. The two programs come from different backgrounds, but do basically the same things internally and have the same optimizations applied. OpenBLAS on this machine is a bit slower than the Accelerate version so that explains *some* of it. OpenNEC also has some differences in the way it sends geometery into the calculation side, it is wavelength based in places that nec2++ uses meters, but that shouldn't have much effect here. More experimentation is warranted.

Small deck effects
------------------

Because these advanced libraries all require time and memory to get up and running, it's possible that they actually slow things down in the case of small decks when the calculation time is so short its best to avoid all of it. But in repeated measurements this appears to be such a small effect as to not worry about it.

The following numbers are a series of decks with the same geometry but different number of segments.

| Deck  | Segments | Reuse off | Reuse on | Effect      |
| ----- | -------: | --------: | -------: | ----------- |
| r0.2  |      534 |  26.7 ms  |  29.1 ms | 8.8% slower |
| r0.5  |    1,118 |  71.1 ms  |  76.8 ms | 8.0% slower |
| r0.8  |    2,126 | 214.8 ms  | 208.2 ms | 3.1% faster |
| r1.0  |    3,078 | 444.1 ms  | 423.1 ms | 4.7% faster |
| full  |    4,182 | 895.7 ms  | 853.7 ms | 4.7% faster |

As you can see, while the smaller decks do indeed run slower, about 8%, the actual time is so small that the difference is invisible.

Putting it all together
-----------------------

To illustrate the performance improvements across a broad selection of different deck complexity and sizes, as well as the improvements the various upgrades offer, a (lengthy!) test was run against a selection of files. These included the original example5.nec from cocoaNEC that was used to validate early versions of OpenNEC, the golden_decks selection of test files, the sym4-series ladder decks, and finally the Whip and LogYag files from Ron's collection. That's 25 decks in total.

Each deck was run through a series of different versions of OpenNEC with various options turned on or off. They start with the basic code as it is in nec2c, and then BLAS, OpenMP, both, and both with full reuse. So each deck ran under five configurations.

| Configuration | Matrix solver             | OpenMP fill | Fill reuse |
| ------------- | ------------------------- | ----------- | ---------- |
| 1             | Original NEC-2 routines   | Off         | Off        |
| 2             | Apple Accelerate          | Off         | Off        |
| 3             | Original NEC-2 routines   | 8 threads   | Off        |
| 4             | Apple Accelerate          | 8 threads   | Off        |
| 5             | Apple Accelerate          | 8 threads   | On         |

To help reduce testing noise, each test was run five times and the values below are averages. So that's 625 runs in total.

| Deck                      | Segments | Frequencies | 1 Classic  | 2 Accelerate | 3 Classic + OMP | 4 Accelerate + OMP | 5 + reuse |
| ------------------------- | -------: | ----------: | ---------: | -----------: | --------------: | -----------------: | --------: |
| example5                  |       78 |           1 |   0.005872 |     0.006771 |        0.006590 |           0.008572 |  0.008902 |
| 01_dipole_free            |        9 |           1 |   0.003672 |     0.007458 |        0.005546 |           0.006650 |  0.007464 |
| 02_gn1_pec                |        9 |           1 |   0.003360 |     0.004547 |        0.004553 |           0.006351 |  0.006389 |
| 03_gn2_sommerfeld         |        9 |           1 |   0.020411 |     0.021715 |        0.022036 |           0.023652 |  0.023566 |
| 04_ld0_series_rlc         |        9 |           1 |   0.003486 |     0.004584 |        0.005144 |           0.006568 |  0.006711 |
| 05_ld5_conductivity       |        9 |           1 |   0.003353 |     0.004556 |        0.004757 |           0.006307 |  0.006353 |
| 06_junction_three_wire    |       15 |           1 |   0.003344 |     0.004473 |        0.004778 |           0.006305 |  0.006293 |
| 07_tl                     |       18 |           1 |   0.003756 |     0.004847 |        0.004935 |           0.006443 |  0.006593 |
| 08_nt                     |       18 |           1 |   0.003419 |     0.004771 |        0.004830 |           0.006388 |  0.006278 |
| 09_fr_sweep               |        9 |           3 |   0.003538 |     0.004978 |        0.005018 |           0.006907 |  0.007104 |
| 10_gx_symmetry            |       18 |           1 |   0.003531 |     0.004875 |        0.005270 |           0.006845 |  0.006989 |
| 11_gn2_low_height         |       21 |           1 |   0.020797 |     0.022195 |        0.021979 |           0.024198 |  0.023861 |
| 12_issue34_near_far       |       11 |           1 |   0.003827 |     0.004885 |        0.005295 |           0.006757 |  0.006740 |
| 13_issue35_rp_xnda        |       11 |           1 |   0.017038 |     0.017960 |        0.018195 |           0.020492 |  0.020833 |
| 14_issue36_rp_ground_mode |       10 |           1 |   0.003636 |     0.004973 |        0.004936 |           0.006945 |  0.006409 |
| 15_issue37_ek_zero        |       11 |           1 |   0.003572 |     0.004498 |        0.004899 |           0.006535 |  0.006605 |
| 16_issue38_ld4_power      |       21 |           1 |   0.003510 |     0.004572 |        0.004933 |           0.006605 |  0.006551 |
| 17_tl_crossed             |       18 |           1 |   0.003465 |     0.004788 |        0.004894 |           0.006489 |  0.006742 |
| sym4_r0.2                 |      534 |          13 |   1.103310 |     0.561523 |        0.732852 |           0.193244 |  0.151904 |
| sym4_r0.5                 |     1118 |          13 |   7.505479 |     2.427248 |        5.799531 |           0.690577 |  0.491863 |
| sym4_r0.8                 |     2126 |          13 |  44.444948 |     8.196248 |       39.014771 |           2.317128 |  1.584319 |
| sym4_r1.0                 |     3078 |          13 | 132.836280 |    16.109393 |      121.984809 |           5.152093 |  3.817467 |
| sym4_full                 |     4182 |          13 | 339.257051 |    32.060547 |      321.226487 |          10.805885 |  8.628106 |
| LogYag                    |     1353 |          65 |  65.097981 |    10.533438 |       59.910743 |           4.785790 |  4.576293 |
| Whip                      |     4392 |          13 | 383.550718 |    30.785950 |      364.814280 |          11.699242 |  9.463203 |

Crunching the numbers, the average overhead for using BLAS and OpenMP is about 0.003 seconds. That is absolutely a cost worth paying. Reuse, which only applies in certain cases as could be a downside on many decks, turns out to take about 0.0008 seconds on average, but that's so small it might be statistical noise. Based on these results, turning on and leaving on all of these features is a good idea.

The simpler decks are at the top. These all have a single frequency and low segment counts so these improvements don't really help. Yet even in these examples the cost is too low to worry about. For instance, example5 takes 0.006 seconds to run using the classic code, and turning on all the features makes that rise to 0.009 seconds, about 50% longer - but the real-world performance is still "effectively instantaneous" from the user's perspective The worse case in the single-frequency examples is 11_gn2_low_height, which takes 0.020797 in classic and 0.023861 with everything on. That's still well within the area that the user will be able to use these in real-time.

At the bottom of the chart we see what this 0.003 second buys us, a massive improvement in large-deck and multi-frequency runs. BLAS is certainly the biggest winner, reducing runs five to twelve times. OpenMP filling also helps, but really shines when it's combined with BLAS. Total improvements on the order of 30 times are typical. sym4_r0.2, which has ~500 segments and a dozen frequencies seems like a pretty typical real-world deck; it takes 1.1 seconds in the classic code and only 0.15 seconds with everything on. This makes this fast enough for a GUI program to render real-time.

So the end result seems clear: except for the very simplest and smallest decks, these additions offer very large improvements, and even on the simplest cases the downside is too tiny to worry about. 

Reproduction
------------

To reproduce these numbers on your machine, build the production Accelerate + OpenMP executable on macOS or follow the directions in BUILD and BUILD-WINDOWS for other platforms:

```sh
make clean
make CC=clang RELEASE=1 BACKEND=accelerate OPENMP=1 \
  OPENMP_CFLAGS='-I/opt/homebrew/opt/libomp/include -Xpreprocessor -fopenmp' \
  OPENMP_LDFLAGS='-L/opt/homebrew/opt/libomp/lib -lomp'
```

Once it's built, you can use the script in the testing folder to run the five-trial benchmark at eight threads:

```sh
OMP_NUM_THREADS=8 VECLIB_MAXIMUM_THREADS=8 \
  tests/benchmark_tests/benchmark_decks.sh ./onec
```
You can change the number of times to run the file inside the script, and you'll likely want to do so when you're running some of the slower engines like nec2c.
