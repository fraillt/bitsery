# Compiler specific patches

This folder will provide patches for various C++ compilers that are not C++11 compatible yet.

Patch can be applied either with `git apply` or `patch` command, like this:
```bash
git apply patches/<patch_name>
patch -p1 < patches/<patch_name>
```

* [centos7_gcc4.8.2.diff](centos7_gcc4.8.2.diff) in this version, unordered_map was not fully C++11 compatible yet. It was lacking some constructors that accepts allocator, and wasn't using `std::allocator_traits`.