## prerequisite
```
# install tirpc
$ sudo yum install libtirpc-devel

# Download "lmbench-3.0-a1"

# modify the build script
# refer: https://github.com/intel/lmbench/issues/1
# refer: https://pagure.io/kernel-tests/issue/25
$ cd $LMBENCH_DIR
$ path -s -p0 < make-runnable.patch
```
