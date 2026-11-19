# OpenNEC performance

Roger Cox, W8IO, publishes a [NEC Benchmark page](https://www.w8io.com/nec-benchmarks.htm) that lists the time to run two large decks using different NEC engines and machines. The first file is a whip with a hand-crafted ground plane with many segments, and the second is a complex log-periodic Yagi. The LogYag is a more typical antenna model, and the relative performance between engines on that file will likely match your own decks.

OpenNEC has added several performance improvements using these decks to guide development, including BLAS/LAPACK support and OpenMP matrix filling. This makes it among the fastest NEC engines available.

The decks and the benchmark script used to measure them can be found in [`tests/benchmark_tests/`](../tests/benchmark_tests/). See the instructions at the end on how to run these on your platform.

## Some background

The original idea behind OpenNEC was to make a version of the nec2c code that could be compiled directly into GUI programs. Previous programs like 4nec2 wrote out a NEC deck, called an external program to run the calculations and write a report file, and then read and parsed that file for display. This round-trip introduced delays, which were significant for smaller antenna designs where the time needed to launch the program was often as long as the calculations. By allowing the functionality to be called directly from the GUI, thereby removing the program startup time, OpenNEC would make GUI programs much faster and more interactive.

A number of authors have also attacked the performance of the underlying NEC engine itself. A common way is to replace NEC's original hand-crafted Fortran matrix math code with fast multi-threaded math libraries. These have the major advantage of being able to run in parallel across multiple cores (sub-processors), which is a feature of all modern computers. NEC's calculations are "embarrassingly parallel", and it speeds up roughly with the number of threads.

By the time the OpenNEC project started, most of these libraries had gone away in favour of OpenBLAS, and this was included in macOS, so this made everything that much easier. Modifying the original nec2c matrix code to call BLAS was relatively simple, and was included in the very first versions of OpenNEC. This resulted in a major boost in performance, on the order of 3 times for most files, and as much as 7 times for very large decks on my 8-core machine. Similar code for Windows was added starting with the 2.2 releases.

Steven Burns, KK7KNB, has been modifying the nec2++ code with similar goals. In additionl to BLAS, this branch added OpenMP support to allow a particular operation, the matrix fill, to also run in parallel. This greatly speeds the setup of complex designs, before BLAS is let loose. He ported these changes back to OpenNEC, and with some further tweaks to make the C gods happy, nec2++ and OpenNEC now offer similar performance on most decks. Both are *dramatically* faster than older engines, often on the order of 10 to 30 times!

## Baseline performance

To provide a performance baseline, I attempted to run these decks with the original Fortran code from the [nec2d-SX source code](https://www.qsl.net/4nec2/supfiles.htm) found on the 4nec2 web pages, with the segment count increased to 11k and the in-deck comments stripped out. However, neither deck would run, reporting `SEGMENT CONNECTION ERROR...`. This turned out to be due to a fixed limit on the number of connections between segments, 60, which both of these files had issues with at one point or another. This issue was fixed in nec2c, so I moved on.

nec2c is a relatively direct port of the original NEC-2 Fortran code, originally using an automated conversion tool. Its times are very similar to the same decks run under nec2d (when they work!), so it can be used as the baseline as well. Here is nec2c 1.3 running these tests on an M2 Max Studio.

| Deck   | Seconds |
|--------|--------:|
| Whip   | 396.52  |
| LogYag | 64.68   |

These numbers are used as the baseline in the following sections. "Times faster" is the number of times faster than nec2c. Larger values are better - 10 means ten times as fast, 5 means only five times as fast.

## Previous best

Cox's page lists a number of different engines and platforms. The fastest engine at the time of the last update, in June 2023, was Athan Papadimitriou's [NEC2/MP 1.5](https://web.archive.org/web/20130502013310/http://users.otenet.gr/~jmsp/) being run on a Core i9-12900H machine with six performance cores.

These numbers are not *directly* compariable to the nec2c results above, as it's a different machine. GeekBench states the M2 Max is about 12 to 25% faster, so accounting for that, the times faster should be closer to something around the 13 area for the whip, and 1.25 for LogYag - those numbers would be expected if we could run NEC/MP on the M2 Max.

| Deck   | Seconds | Times faster |
|--------|--------:|-------------:|
| Whip   | 38      | 10.4         |
| LogYag | 67.8    | 0.95         |

These results show the improvement you get from using the BLAS code, which allows the mathematics to run across the multiple cores of the machine and gives a 10 times performance boost to the calculations, which is pretty good considering it has 12 cores, and seems to suggest it was using all of them, not just the performance cores. However, it offers little advantage in the LogYag case where the performance is mostly hung up in the building of the geometry, which NEC2/MP didn't change.

## OpenNEC results

Now let's compare those results with OpenNEC, which has BLAS and has also implemented the changes to the matrix fill using OpenMP. The numbers are from version 2.4.1 built as a production binary using the clang complier, `-O2` optimization, Apple Accelerate (which contains BLAS) and OpenMP. Both decks were run five times with `OMP_NUM_THREADS=8` and `VECLIB_MAXIMUM_THREADS=8`, which matches the 8-core system, and the times below are the fastest of the five.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| Whip   | 12.59   | 31.5  |
| LogYag | 4.69    | 13.8  |

OpenMP is as much as 30 times faster than the baseline, and about three times faster than the previous record holder, NEC/MP. 

You can clearly see the effect of threading the matrix fill in these numbers. In NEC/MP, the LogYag ends up taking almost twice as long as the whip. But by threading the setup, in OpenNEC it runs over twice as fast as whip. This means there's a four-fold overall improvement, suggesting something close to eight-fold improvement on the setup half, about what we would expect on an 8-core machine.

## NEC2++ comparison

Many of the performance upgrades to OpenNEC were recommended by Steven Burns, who implemented similar changes in his fork of nec2++ which is part of the larger [momwire](https://github.com/stevenmburns/momwire) project. These are single-run measurements using momwire's nec2++ code built on the M2 with OpenBLAS, LAPACK and OpenMP, and `OMP_NUM_THREADS=8` and `OPENBLAS_NUM_THREADS=8`, as in the OpenNEC examples. The whip was run from a temporary copy with apostrophe comments removed, and the wire-intersection check was disabled because it rejects the mesh.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| Whip   | 18.43   | 21.5  |
| LogYag | 9.28    | 7.0   |


The modified nec2++ code includes one additional feature that OpenNEC currently lacks, a system that can re-use certain parts of the internal mathematics depending on how the geometry is set up, a feature called "fill reuse". The following times are with that feature turned on.

| Deck   | Seconds | Times faster |
|--------|--------:|------:|
| Whip   | 13.60   | 29.2  |
| LogYag | 10.40   | 6.2   |

Reuse substantially reduced the whip time, because the ground plane's geometry matched the requirements for reuse. LogYag exceeded the reuse limit, so it falls back to the ordinary fill. This adds a small amount of overhead, but not enough to explain the extra second, which is likely just testing noise.

As you can see, the reuse offers a real boost in performance, and is a target for inclusion in OpenNEC.

## Thread settings

One annoyance of the OpenMP system is that its performance with NEC is best when the number of threads is equal to the number of physical cores. Unfortunately, there's no standard way to know what that is, it differs across platforms and many CPUs have different sorts of cores which may or may not be returned in the count.

So to get OpenNEC to deliver the best peformance for your system, you'll have to look up your machine's specs on the 'net, and look for the "performance cores" number (or something like that, the name varies too). You can then specify that number of threads when you run the program using command-line variables.

For instance, on a machine with eight performance cores, you would want to try:

`OMP_NUM_THREADS=8 VECLIB_MAXIMUM_THREADS=8 ./onec` ...

You'll want to experiment with the numbers, some machines prefer a larger number than the number of cores, and others strongly prefer smaller, but the majority seem to want "threads = cores".

## Reproduction

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
