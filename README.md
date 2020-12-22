# hazytrip
a tripcode bruteforcer for Futaba-type imageboards
#

hazytrip is a CPU-intensive tripcode bruteforcer for [Futaba](https://en.wikipedia.org/wiki/2channel)-type imageboards. it is written in C.
it achieves its goal of bruteforcing tripcodes by generation random Shift-JIS compatible password strings, using all (at the time of running) usable CPU cores and prints tripcode matches to stdout.

#### about
multiple tripcode search programs try to produce password strings with multi-byte Shift-JIS characters, but some modern imageboard systems convert exotic character sets to UTF-8, causing the password produced useless

hazytrip removes this possibility by limiting the character range to US-ASCII except for ```\``` and ```~```, which have identical code points in Shift-JIS and UTF-8.

hazytrip's main usage is for generating vanity tripcodes, but since it attempts to use your CPU's **full** capacity, it may also be used as a performance tester.

### usage
```
usage:
	hazytrip [OPTION] "SEARCH STRING"
help:
  (None)  no query. hazytrip will print random tripcodes to stdout.
  -i      case agnostic search.
  -h      display this help screen.
note:
	tripcodes can be no longer than 10 characters
	tripcodes can only have the range ./0-9A-Za-z in them
	the 10th character of a tripcode can only be one of these characters, you know! .26AEIMQUYcgkosw
```

#### search speed
at extreme speeds, search functions can become a liability; hazytrip provides a lightweight native search function, but if you would prefer some more implemented granular control, such as `RegEx`, you can pipe the output of hazytrip to `grep`, however, this is a huge bottleneck, reducing overall speed by **at least *30%***.

output speed may also become a small liability if hazytrip prints search string matches to stdout faster than your terminal can print stdout results

it's a good idea to note that case-sensitive searches are noticeably faster than case-agnostic searches and the smaller your search string, the faster you will get matches.

#### "Can I use this for secure tripcodes?"
not unless you know the secret salt of the imageboard system you're targeting

#### Installation/Building
there are two ways of doing this; building the .c file yourself or getting the binary from the [releases](https://github.com/derelictpillows/hazytrip/releases) tab

**building yourself**
```
git clone https://github.com/derelictpillows/hazytrip
cd hazytrip
make
./hazytrip
```

if you want to "install it", just do this after running `make`

```sudo cp hazytrip /usr/bin```

you can delete the hazytrip folder afterwards if you do this

#### dependencies
`libssl` for tripcode hashing using DES, make sure you have it installed before you run hazytrip

`OpenMP`, a.k.a. `clang-omp` on OS X for multithreading ***(optional)***

### support
you can support this program if you care enough by just opening an issue or pull request if you find any bugs. that's all

### copyright and license
this project is public domain. no copyright attached.

<sub>keywords: tripcode, bruteforcer, searcher, cracker, finder, explorer, generator, imageboard, textboard, 双葉ちゃん, futaba, 2ch, 2chan, yotsuba, 4ch, 4chan</sub>
