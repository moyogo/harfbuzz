## Build & Run

Depending on what area you are working in change or add `HB_DEBUG_<whatever>`.
Values defined in `hb-debug.hh`.

```shell
# quick sanity check
time (make CPPFLAGS='-DHB_DEBUG_SUBSET=100' \
  && make -C test/api check || cat test/api/test-suite.log)

# slower santiy check
time (make CPPFLAGS='-DHB_DEBUG_SUBSET=100' \
   && make -C src check \
   && make -C test/api check \
   && make -C test/subset check)

# confirm you didn't break anything else
time (make CPPFLAGS='-DHB_DEBUG_SUBSET=100' \
  && make check)

# often catches files you didn't add, e.g. test fonts to EXTRA_DIST
make distcheck
```

### Debug with GDB

```
cd ./util
../libtool --mode=execute gdb --args ./hb-subset ...
```

### Enable Debug Logging

```shell
# make clean if you previously build w/o debug logging
make CPPFLAGS=-DHB_DEBUG_SUBSET=100
```

## Build and Test via CMake

Note: You'll need to first install ninja-build via apt-get.

```shell
cd harfbuzz
mkdir buid
cmake -DHB_CHECK=ON -Bbuild -H. -GNinja && ninja -Cbuild && CTEST_OUTPUT_ON_FAILURE=1 ninja -Cbuild test
```
## Test with the Fuzzer

```shell
# push your changs to a branch on googlefonts/harfbuzz
# In a local copy of oss-fuzz, edit projects/harfbuzz/Dockerfile
# Change the git clone to pull your branch
sudo python infra/helper.py build_image harfbuzz
sudo python infra/helper.py build_fuzzers --sanitizer address harfbuzz
sudo python infra/helper.py run_fuzzer harfbuzz hb-subset-fuzzer
```
